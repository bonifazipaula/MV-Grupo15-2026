#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>     //forzar tipos de 32 bits

#include "MV.h"
#include "Funciones.h"
#include "Disassembler.h"
#include "Registros.h"
#include "Segmentos.h"

//  funciones para leer en Big Endian
static unsigned short int leerUint16BE(FILE *arch)
{
    unsigned char buf[2];
    fread(buf, 1, 2, arch);
    return (buf[0] << 8) | buf[1];
}

static void escribirUint16BE(FILE *arch, unsigned short int val)
{
    unsigned char buf[2];
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
    fwrite(buf, 1, 2, arch);
}

static void escribirInt32BE(FILE *arch, int32_t val)
{
    unsigned char buf[4];
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
    fwrite(buf, 1, 4, arch);
}

static int32_t leerInt32BE(FILE *arch)
{
    unsigned char buf[4];
    fread(buf, 1, 4, arch);
    return ((int32_t)buf[0] << 24) | ((int32_t)buf[1] << 16) |
           ((int32_t)buf[2] << 8) | (int32_t)buf[3];
}


void inicializarMV(MV *mv, int tamMemoria)
{
    int i;

    mv->tamRAM = tamMemoria;
    mv->RAM = (unsigned char *)malloc(tamMemoria);
    if (mv->RAM == NULL)
        errorMV("No se pudo asignar memoria para la RAM");

    memset(mv->RAM, 0, tamMemoria);

    for (i = 0; i < cantReg; i++)
        mv->tabla_de_registros[i] = 0;

    for (i = 0; i < cantSeg; i++)
    {
        mv->tabla_de_segmentos[i].base = 0xFFFF;
        mv->tabla_de_segmentos[i].tam = 0xFFFF;
    }

    mv->archivoVMI = NULL;
    mv->stepMode = 0;
}

void liberarMV(MV *mv)
{
    if (mv->RAM != NULL)
    {
        free(mv->RAM);
        mv->RAM = NULL;
    }
}

// ahora tenemos más argunentos que vienne del comando
//  vmx [filename.vmx] [filename.vmi] [m=M] [-d] [-p param1 param2 ... paramN]

// Verifica si un string termina con una extensión dada
static int tieneExtension(const char *nombre, const char *ext)
{
    size_t lenNombre = strlen(nombre);
    size_t lenExt = strlen(ext);
    if (lenNombre < lenExt)
        return 0;
    return strcmp(nombre + lenNombre - lenExt, ext) == 0;
}

void parsearArgumentos(int argc, char *argv[], Parametros *params)
{
    int i;

    // Valores por defecto
    params->archivoVMX = NULL;
    params->archivoVMI = NULL;
    params->tamMemoria = TamRamDefault;
    params->flagDisassembler = 0;
    params->argc = 0;
    params->argv = NULL;
    params->argvStart = -1;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            // Todo lo que sigue son parámetros del programa
            // argc incluye el nombre del vmx como primer parámetro
            params->argvStart = i + 1;
            params->argc = argc - (i + 1);
            if (params->archivoVMX != NULL)
                params->argc++; // +1 por el nombre del vmx (argv[0] del programa)
            params->argv = argv;
            break; // -p siempre es el último flag!!
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            // aHORA la MV tiene el flag del Disassembler en vez de llamar al método
            params->flagDisassembler = 1;
        }
        else if (strncmp(argv[i], "m=", 2) == 0)
        {
            int kib = atoi(argv[i] + 2);
            if (kib <= 0)
                errorMV("Tamaño de memoria invalido");
            params->tamMemoria = kib * 1024;
        }
        else if (tieneExtension(argv[i], ".vmx"))
        {
            params->archivoVMX = argv[i];
        }
        else if (tieneExtension(argv[i], ".vmi"))
        {
            params->archivoVMI = argv[i];
        }
        else
        {
            fprintf(stderr, "Argumento no reconocido: %s\n", argv[i]);
            errorMV("Uso: vmx [archivo.vmx] [archivo.vmi] [m=M] [-d] [-p param1 ... paramN]");
        }
    }

    // ahora tenemos archivo .vmi y .vmx
    // Validar que al menos uno de los dos archivos esté presente
    if (params->archivoVMX == NULL && params->archivoVMI == NULL)
        errorMV("Debe indicarse al menos un archivo .vmx o .vmi");

    // Si no hay .vmx, se ignoran los parámetros -p
    if (params->archivoVMX == NULL)
    {
        params->argc = 0;
        params->argv = NULL;
        params->argvStart = -1;
    }
}

// El Param Segment consta de varios segmentos
static int construirParamSegment(MV *mv, Parametros *params, int baseParamSeg)
{
    int i;
    int tamStrings = 0;
    int numParams;
    int posEscritura;

    if (params->archivoVMX == NULL)
        return 0;

    // El primer string es el nombre del archivo vmx
    // Luego vienen los parámetros de -p (si los hay)

    if (params->argc == 0)
        return 0; // No hay -p, no se crea Param Segment

    // Calcular tamaño de los strings
    // argv[0] del programa = nombre del vmx
    tamStrings += strlen(params->archivoVMX) + 1;

    numParams = params->argc;

    // Parámetros de -p (sin contar el nombre del vmx)
    for (i = params->argvStart; i < params->argvStart + (numParams - 1); i++)
        tamStrings += strlen(params->argv[i]) + 1;

    int tamArreglo = numParams * 4;
    int tamTotal = tamStrings + tamArreglo;

    if (baseParamSeg + tamTotal > mv->tamRAM)
        errorMV("Memoria insuficiente para el Param Segment");

    // Escribir los strings en la memoria
    posEscritura = baseParamSeg;

    // Guardar offsets de cada string para construir el arreglo argv
    int *offsets = (int *)malloc(numParams * sizeof(int));
    if (offsets == NULL)
        errorMV("Error de memoria al construir Param Segment");

    // String 0: nombre del archivo vmx
    offsets[0] = posEscritura - baseParamSeg;
    int len = strlen(params->archivoVMX);
    memcpy(&mv->RAM[posEscritura], params->archivoVMX, len + 1);
    posEscritura += len + 1;

    // Strings 1..N: parámetros de -p
    for (i = 0; i < numParams - 1; i++)
    {
        offsets[i + 1] = posEscritura - baseParamSeg;
        len = strlen(params->argv[params->argvStart + i]);
        memcpy(&mv->RAM[posEscritura], params->argv[params->argvStart + i], len + 1);
        posEscritura += len + 1;
    }

    // Escribir el arreglo argv (punteros de 4 bytes, segmento 0, offset = posición del string)
    for (i = 0; i < numParams; i++)
    {
        uint32_t puntero = (uint32_t)(offsets[i] & 0xFFFF);
        mv->RAM[posEscritura]     = (puntero >> 24) & 0xFF;
        mv->RAM[posEscritura + 1] = (puntero >> 16) & 0xFF;
        mv->RAM[posEscritura + 2] = (puntero >> 8) & 0xFF;
        mv->RAM[posEscritura + 3] = puntero & 0xFF;
        posEscritura += 4;
    }

    free(offsets);
    return tamTotal;
}

// Manejo de la pila como se explica en la pág 4 de la especificación V2
static void pushEnPila(MV *mv, int32_t valor)
{
    mv->tabla_de_registros[SP] -= 4;

    int32_t spVal = mv->tabla_de_registros[SP];
    uint16_t segmento = (spVal >> 16) & 0xFFFF;
    uint16_t offset = spVal & 0xFFFF;

    if ((int)offset < 0 || segmento >= cantSeg)
        errorMV("Stack overflow");

    // Verificar que SP no baje del inicio del Stack Segment
    int32_t ssVal = mv->tabla_de_registros[SS];
    uint16_t ssOffset = ssVal & 0xFFFF;
    if (offset < ssOffset)
        errorMV("Stack overflow");

    int dirFisica = mv->tabla_de_segmentos[segmento].base + offset;

    mv->RAM[dirFisica]     = (valor >> 24) & 0xFF;
    mv->RAM[dirFisica + 1] = (valor >> 16) & 0xFF;
    mv->RAM[dirFisica + 2] = (valor >> 8) & 0xFF;
    mv->RAM[dirFisica + 3] = valor & 0xFF;
}

// Cargar el vmx a la memoria
void cargarPrograma(MV *mv, Parametros *params)
{
    FILE *Arch;
    char identificador[6];
    unsigned char version;

    Arch = fopen(params->archivoVMX, "rb");
    if (Arch == NULL)
        errorMV("No se pudo abrir el archivo .vmx");

    fread(identificador, sizeof(char), 5, Arch);
    identificador[5] = '\0';

    if (strcmp(identificador, "VMX26") != 0)
        errorMV("Archivo VMX invalido: identificador erroneo");

    fread(&version, sizeof(unsigned char), 1, Arch);

    if (version == 1)
    {
        unsigned short int tamCodigo = leerUint16BE(Arch);

        if (tamCodigo > mv->tamRAM)
        {
            fclose(Arch);
            errorMV("El programa es demasiado grande para la memoria");
        }

        // Cargar código en la dirección física 0 de la RAM
        if (fread(mv->RAM, sizeof(unsigned char), tamCodigo, Arch) != tamCodigo)
        {
            fclose(Arch);
            errorMV("No se pudo leer completamente el codigo");
        }
        fclose(Arch);

        mv->tabla_de_segmentos[0].base = 0;
        mv->tabla_de_segmentos[0].tam = tamCodigo;

        mv->tabla_de_segmentos[1].base = tamCodigo;
        mv->tabla_de_segmentos[1].tam = mv->tamRAM - tamCodigo;

        mv->tabla_de_registros[CS] = 0x00000000; 
        mv->tabla_de_registros[DS] = 0x00010000; 
        mv->tabla_de_registros[ES] = 0xFFFFFFFF;
        mv->tabla_de_registros[SS] = 0xFFFFFFFF;
        mv->tabla_de_registros[KS] = 0xFFFFFFFF;
        mv->tabla_de_registros[PS] = 0xFFFFFFFF;

        // IP apunta al inicio del código
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS];
        mv->tabla_de_registros[SP] = 0;
        mv->tabla_de_registros[BP] = 0;
    }
    else if (version == 2)
    {
       // ahora hay más cantidda de segmentos
        unsigned short int tamCodigo   = leerUint16BE(Arch); 
        unsigned short int tamData     = leerUint16BE(Arch);  
        unsigned short int tamExtra    = leerUint16BE(Arch); 
        unsigned short int tamStack    = leerUint16BE(Arch);
        unsigned short int tamConst    = leerUint16BE(Arch);
        unsigned short int entryPoint  = leerUint16BE(Arch);

        // Param Segment: solo si se incluyeron parámetros con -p
        int tamParam = 0;
        if (params->argc > 0)
            tamParam = construirParamSegment(mv, params, 0);

        // Verificar si la memoria alcanza para todos los segmentos del proceso
        int tamTotal = tamParam + tamConst + tamCodigo + tamData + tamExtra + tamStack;
        if (tamTotal > mv->tamRAM)
        {
            fclose(Arch);
            errorMV("Memoria insuficiente");
        }

        // cálculo de las bases de cada segmento
        int baseParam = 0;
        int baseConst = baseParam + tamParam;
        int baseCode  = baseConst + tamConst;
        int baseData  = baseCode + tamCodigo;
        int baseExtra = baseData + tamData;
        int baseStack = baseExtra + tamExtra;

        // Cargar Code Segment
        if (fread(&mv->RAM[baseCode], sizeof(unsigned char), tamCodigo, Arch) != tamCodigo)
        {
            fclose(Arch);
            errorMV("No se pudo leer completamente el codigo");
        }

        if (tamConst > 0)
        {
            if (fread(&mv->RAM[baseConst], sizeof(unsigned char), tamConst, Arch) != tamConst)
            {
                fclose(Arch);
                errorMV("No se pudo leer completamente el Const Segment");
            }
        }
        fclose(Arch);

        // Construir tabla de descriptores de segmentos:
        // Solo entradas para segmentos con tamaño > 0, en orden físico
        int indiceSeg = 0;
        int indiceParamSeg = -1, indiceConstSeg = -1, indiceCodeSeg = -1;
        int indiceDataSeg = -1, indiceExtraSeg = -1, indiceStackSeg = -1;

        if (tamParam > 0) {
            indiceParamSeg = indiceSeg;
            mv->tabla_de_segmentos[indiceSeg].base = baseParam;
            mv->tabla_de_segmentos[indiceSeg].tam = tamParam;
            indiceSeg++;
        }

        if (tamConst > 0) {
            indiceConstSeg = indiceSeg;
            mv->tabla_de_segmentos[indiceSeg].base = baseConst;
            mv->tabla_de_segmentos[indiceSeg].tam = tamConst;
            indiceSeg++;
        }

        // Code Segment, siempre está!!
        {
            indiceCodeSeg = indiceSeg;
            mv->tabla_de_segmentos[indiceSeg].base = baseCode;
            mv->tabla_de_segmentos[indiceSeg].tam = tamCodigo;
            indiceSeg++;
        }

        if (tamData > 0) {
            indiceDataSeg = indiceSeg;
            mv->tabla_de_segmentos[indiceSeg].base = baseData;
            mv->tabla_de_segmentos[indiceSeg].tam = tamData;
            indiceSeg++;
        }

        if (tamExtra > 0) {
            indiceExtraSeg = indiceSeg;
            mv->tabla_de_segmentos[indiceSeg].base = baseExtra;
            mv->tabla_de_segmentos[indiceSeg].tam = tamExtra;
            indiceSeg++;
        }

        // Stack Segment, siempre está en la MV2
        {
            indiceStackSeg = indiceSeg;
            mv->tabla_de_segmentos[indiceSeg].base = baseStack;
            mv->tabla_de_segmentos[indiceSeg].tam = tamStack;
            indiceSeg++;
        }

        // Inicializar registros de segmento
        mv->tabla_de_registros[PS] = (indiceParamSeg >= 0)
            ? ((long int)indiceParamSeg << 16)
            : 0xFFFFFFFF;

        mv->tabla_de_registros[KS] = (indiceConstSeg >= 0)
            ? ((long int)indiceConstSeg << 16)
            : 0xFFFFFFFF;

        mv->tabla_de_registros[CS] = (long int)indiceCodeSeg << 16;

        mv->tabla_de_registros[DS] = (indiceDataSeg >= 0)
            ? ((long int)indiceDataSeg << 16)
            : 0xFFFFFFFF;

        mv->tabla_de_registros[ES] = (indiceExtraSeg >= 0)
            ? ((long int)indiceExtraSeg << 16)
            : 0xFFFFFFFF;

        mv->tabla_de_registros[SS] = (long int)indiceStackSeg << 16;

        // IP = CS + entry point offset
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] | (entryPoint & 0xFFFF);

        // SP = SS + tamaño del stack (pila vacía, apunta fuera del segmento)
        mv->tabla_de_registros[SP] = mv->tabla_de_registros[SS] | (tamStack & 0xFFFF);

        // Preparar la pila para la subrutina principal:
        // Orden de push (de fondo hacia el tope):
        //   1. argv
        //   2. argc
        //   3. dirección de retorno (-1 / 0xFFFFFFFF)
        if (params->argc > 0)
        {
            int tamStrings = tamParam - (params->argc * 4);
            int32_t argvPtr = ((int32_t)indiceParamSeg << 16) | (tamStrings & 0xFFFF);
            pushEnPila(mv, argvPtr);
            pushEnPila(mv, (int32_t)params->argc);
        }
        else
        {
            pushEnPila(mv, (int32_t)0xFFFFFFFF); // argv = -1
            pushEnPila(mv, 0);                    // argc = 0
        }

        // Dirección de retorno de la subrutina principal = 0xFFFFFFFF
        pushEnPila(mv, (int32_t)0xFFFFFFFF);
    }
    else
    {
        fclose(Arch);
        errorMV("Version VMX invalida");
    }
}

// SI por comandos se ingresó un archivo imagen se carga
void cargarImagen(MV *mv, char *archivoVMI)
{
    FILE *f;
    char identificador[6];
    unsigned char version;
    unsigned short int tamMemKiB;
    int i;

    f = fopen(archivoVMI, "rb");
    if (f == NULL)
        errorMV("No se pudo abrir el archivo .vmi");

    fread(identificador, sizeof(char), 5, f);
    identificador[5] = '\0';

    if (strcmp(identificador, "VMI26") != 0)
    {
        fclose(f);
        errorMV("Archivo VMI invalido: identificador erroneo");
    }

    fread(&version, sizeof(unsigned char), 1, f);
    if (version != 1)
    {
        fclose(f);
        errorMV("Version VMI invalida");
    }

    tamMemKiB = leerUint16BE(f);

    // Realocar la memoria si es necesario
    int tamMemBytes = tamMemKiB * 1024;
    if (tamMemBytes != mv->tamRAM)
    {
        free(mv->RAM);
        mv->tamRAM = tamMemBytes;
        mv->RAM = (unsigned char *)malloc(tamMemBytes);
        if (mv->RAM == NULL)
            errorMV("No se pudo asignar memoria para la RAM");
        memset(mv->RAM, 0, tamMemBytes);
    }

    // Cargar registros (32 x 4 bytes, Big Endian)
    for (i = 0; i < cantReg; i++)
        mv->tabla_de_registros[i] = leerInt32BE(f);

    // Cargar tabla de segmentos (8 x 4 bytes: 2 bytes base + 2 bytes tam)
    for (i = 0; i < cantSeg; i++)
    {
        mv->tabla_de_segmentos[i].base = leerUint16BE(f);
        mv->tabla_de_segmentos[i].tam = leerUint16BE(f);
    }

    // Cargar memoria principal
    if (fread(mv->RAM, sizeof(unsigned char), tamMemBytes, f) != (size_t)tamMemBytes)
    {
        fclose(f);
        errorMV("No se pudo leer completamente la memoria de la imagen");
    }

    fclose(f);
}

// Guardar imagen de la MV
void guardarImagen(MV *mv, char *archivoVMI)
{
    FILE *f;
    int i;

    f = fopen(archivoVMI, "wb");
    if (f == NULL)
        errorMV("No se pudo crear el archivo .vmi");

    // Header
    fwrite("VMI26", sizeof(char), 5, f);
    unsigned char version = 1;
    fwrite(&version, sizeof(unsigned char), 1, f);
    escribirUint16BE(f, (unsigned short int)(mv->tamRAM / 1024));

    // Registros (32 x 4 bytes, Big Endian)
    for (i = 0; i < cantReg; i++)
        escribirInt32BE(f, (int32_t)mv->tabla_de_registros[i]);

    // Tabla de segmentos (8 x 4 bytes)
    for (i = 0; i < cantSeg; i++)
    {
        escribirUint16BE(f, mv->tabla_de_segmentos[i].base);
        escribirUint16BE(f, mv->tabla_de_segmentos[i].tam);
    }

    // Memoria principal
    fwrite(mv->RAM, sizeof(unsigned char), mv->tamRAM, f);

    fclose(f);
}


static int manejarBreakpoint(MV *mv)
{
    if (mv->archivoVMI == NULL)
        return 0; // Sin archivo VMI, ignorar breakpoint

    guardarImagen(mv, mv->archivoVMI);

    char input[64];
    printf("[BREAKPOINT] (g=go, q=quit, Enter=step): ");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL)
        return -1; // quit

    if (input[0] == 'g' || input[0] == 'G')
    {
        mv->stepMode = 0;
        return 0; // continuar
    }
    else if (input[0] == 'q' || input[0] == 'Q')
    {
        return -1; // abortar, VMI ya guardado
    }
    else if (input[0] == '\n')
    {
        mv->stepMode = 1; // paso a paso
        return 0;
    }

    return 0;
}

// Funciones de la MV
int obtenerDirFisica(MV *mv)
{
    int segIdx = (mv->tabla_de_registros[IP] >> 16) & 0xFFFF;
    int offset = mv->tabla_de_registros[IP] & 0xFFFF;
    int dirFisica = mv->tabla_de_segmentos[segIdx].base + offset;
    return dirFisica;
}

int cantidadOperandos(int codOp)
{
    int cantOper;
    if (codOp >= 0x10)
        cantOper = 2;
    else if (codOp <= 0x0A)
        cantOper = 1;
    else if (codOp == 0x0B || codOp == 0x0C || codOp == 0x0D)
        cantOper = 1;
    else if (codOp == 0x0E || codOp == 0x0F)
        cantOper = 0;
    else
        errorMV("Instruccion invalida");

    return cantOper;
}

void obtenerTiposOperandos(unsigned char instruccion, int cantOper, unsigned char *tipoOpA, unsigned char *tipoOpB)
{
    *tipoOpA = 0;
    *tipoOpB = 0;

    if (cantOper == 2)
    {
        *tipoOpB = (instruccion >> 6) & 0x03;
        *tipoOpA = (instruccion >> 4) & 0x03;
    }
    else if (cantOper == 1)
        *tipoOpA = (instruccion >> 6) & 0x03;
}

void cargarOperandos(MV *mv, int dirFisica, int cantOper, unsigned char tipoOpA, unsigned char tipoOpB)
{
    int posOper = dirFisica + 1;
    int i;

    mv->tabla_de_registros[OP1] = 0;
    mv->tabla_de_registros[OP2] = 0;

    if (cantOper == 2) {
        mv->tabla_de_registros[OP2] = ((uint32_t)tipoOpB << 24);

        for (i = 0; i < tipoOpB; i++)
            mv->tabla_de_registros[OP2] |= ((uint32_t)mv->RAM[posOper + i] << (8 * (tipoOpB - 1 - i)));
        posOper += tipoOpB;
    }

    if (cantOper >= 1) {
        mv->tabla_de_registros[OP1] = ((uint32_t)tipoOpA << 24);
        for (i = 0; i < tipoOpA; i++)
            mv->tabla_de_registros[OP1] |= ((uint32_t)mv->RAM[posOper + i] << (8 * (tipoOpA - 1 - i)));
    }
}

int obtenerDirFisicaOperando(MV *mv, long int operando, long int *dirLogicaEfectiva)
{
    unsigned char codReg;
    int offsetFinal, dirFisica;
    long int dirLogica;
    short int desplazamiento;
    unsigned short int segmento, offset;

    codReg = operando & 0x1F;
    desplazamiento = (short int)((operando >> 8) & 0xFFFF);

    dirLogica = mv->tabla_de_registros[codReg];
    segmento = (dirLogica >> 16) & 0xFFFF;
    offset = dirLogica & 0xFFFF;

    offsetFinal = (int)offset + desplazamiento;
    if (segmento >= cantSeg)
        errorMV("Segmento invalido");

    if (mv->tabla_de_segmentos[segmento].base == 0xFFFF && mv->tabla_de_segmentos[segmento].tam == 0xFFFF)
        errorMV("Segmento invalido");

    if (offsetFinal < 0 || offsetFinal + 4 >= mv->tabla_de_segmentos[segmento].tam)
        errorMV("Direccion fuera del segmento");

    *dirLogicaEfectiva = ((long int)segmento << 16) | (offsetFinal & 0xFFFF);
    dirFisica = mv->tabla_de_segmentos[segmento].base + offsetFinal;

    return dirFisica;
}

void leerMemoria(MV *mv, long int operando, long int *valor)
{
    int dirFisica;
    long int dirLogica;

    dirFisica = obtenerDirFisicaOperando(mv, operando, &dirLogica);

    mv->tabla_de_registros[LAR] = dirLogica;
    mv->tabla_de_registros[MAR] = ((long int)4 << 16) | (dirFisica & 0xFFFF);

    *valor = 0;

    *valor |= ((long int)mv->RAM[dirFisica] << 24);
    *valor |= ((long int)mv->RAM[dirFisica + 1] << 16);
    *valor |= ((long int)mv->RAM[dirFisica + 2] << 8);
    *valor |= ((long int)mv->RAM[dirFisica + 3]);
    mv->tabla_de_registros[MBR] = *valor;
}

/* obtengo valor de op1 / op2*/
void obtenerValorOperando(MV *mv, uint32_t operando, int32_t *valor)
{
    unsigned char tipo = (operando >> 24) & 0xFF;
    unsigned char codReg;

    if (tipo == 1) // REGISTRO
    {
        codReg = operando & 0x1F;
        *valor = (int32_t)mv->tabla_de_registros[codReg];
    }
    else if (tipo == 2) // INMEDIATO
    {
        *valor = (int16_t)(operando & 0xFFFF);
    }
    else if (tipo == 3) // MEMORIA
    {
        long int valorTemp;
        leerMemoria(mv, operando, &valorTemp);
        *valor = (int32_t)valorTemp;
    }
    else if (tipo != 0) // Si es 0 (ninguno) no hace nada
    {
        errorMV("Tipo de operando erroneo");
    }
}

void escribirMemoria(MV *mv, long int operando, long int valor)
{
    int dirFisica;
    long int dirLogica;

    dirFisica = obtenerDirFisicaOperando(mv, operando, &dirLogica);

    mv->tabla_de_registros[LAR] = dirLogica;
    mv->tabla_de_registros[MAR] = ((long int)4 << 16) | (dirFisica & 0xFFFF);

    mv->RAM[dirFisica] = (valor >> 24) & 0xFF;
    mv->RAM[dirFisica + 1] = (valor >> 16) & 0xFF;
    mv->RAM[dirFisica + 2] = (valor >> 8) & 0xFF;
    mv->RAM[dirFisica + 3] = valor & 0xFF;

    mv->tabla_de_registros[MBR] = valor;
}

void guardarValorOperando(MV *mv, long int operando, long int valor)
{
    unsigned char tipo, codReg;
    tipo = (operando >> 24) & 0xFF;
    if (tipo == 1)
    {
        codReg = operando & 0x1F;
        mv->tabla_de_registros[codReg] = valor;
    }
    else if (tipo == 3)
        escribirMemoria(mv, operando, valor);
    else
        errorMV("Operando destino invalido");
}

void ejecutarInstruccion(MV *mv, VectorFunciones vecF)
{
    vecF[mv->tabla_de_registros[OPC]](mv);
}

void ejecutarPrograma(MV *mv, int flagDisassembler)
{
    int dirFisica, cantOper, tamInstr;
    unsigned char tipoOpA, tipoOpB, instruccion;
    VectorFunciones vecF;

    // Obtener el índice del Code Segment para verificar fin de ejecución
    int csIdx = (mv->tabla_de_registros[CS] >> 16) & 0xFFFF;

    iniciaVectorFunciones(vecF);
    while ((mv->tabla_de_registros[IP] != (long int)0xFFFFFFFF) &&
           ((mv->tabla_de_registros[IP] & 0xFFFF) < mv->tabla_de_segmentos[csIdx].tam))
    {
        dirFisica = obtenerDirFisica(mv);
        instruccion = mv->RAM[dirFisica];
        mv->tabla_de_registros[OPC] = instruccion & 0x1F;
        cantOper = cantidadOperandos(mv->tabla_de_registros[OPC]);
        obtenerTiposOperandos(instruccion, cantOper, &tipoOpA, &tipoOpB);

        tamInstr = 1 + tipoOpA + tipoOpB;

        cargarOperandos(mv, dirFisica, cantOper, tipoOpA, tipoOpB);
        mv->tabla_de_registros[IP] += tamInstr;

        if (flagDisassembler)
            muestraInstruccion(mv, dirFisica, tamInstr);

        ejecutarInstruccion(mv, vecF);

        // Modo paso a paso (después de cada instrucción, hacer breakpoint)
        if (mv->stepMode && mv->tabla_de_registros[IP] != (long int)0xFFFFFFFF)
        {
            if (manejarBreakpoint(mv) < 0)
                break; // quit
        }
    }
}
