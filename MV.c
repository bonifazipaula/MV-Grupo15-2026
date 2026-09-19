#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MV.h"
#include "Funciones.h"
#include "Disassembler.h"
#include "Registros.h"

void inicializarMV(MV *mv)
{   int i;
    memset(mv->RAM, 0, sizeof(mv->RAM));
    for (i = 0; i < cantReg; i++)
    {   
        mv->tabla_de_registros[i] = 0;
    }

    for (i = 0; i < cantSeg; i++)
    {
       mv->tabla_de_segmentos[i].base = 0xFFFF;
       mv->tabla_de_segmentos[i].tam = 0xFFFF;
    }
}
void cargarPrograma(MV *mv, char *nombreArchivo)
{ FILE *Arch;
  char identificador[6];
  unsigned char version;
  unsigned short int tamCodigo;

  Arch=fopen(nombreArchivo, "rb");
  if (Arch == NULL)
     errorMV("No se pudo abrir el archivo");
  else
  {  fread(identificador,sizeof(char),5,Arch);
     identificador[5] = '\0';
    
     if (strcmp(identificador, "VMX26") != 0)
       errorMV("Archivo VMX invalido identificador erroneo");
     else
     {    fread(&version,sizeof(unsigned char),1,Arch);
          if(version!=1)
            errorMV("Version VMX invalida");
          else
          {  fread(&tamCodigo, sizeof(unsigned short int), 1, Arch);  
             if (tamCodigo > TamRam)
                 errorMV("El programa es demasiado grande para la memoria");
             if (fread(mv->RAM, sizeof(unsigned char), tamCodigo, Arch) != tamCodigo)
                 errorMV("No se pudo leer completamente el codigo");

            mv->tabla_de_segmentos[0].base = 0;
            mv->tabla_de_segmentos[0].tam = tamCodigo;

            mv->tabla_de_segmentos[1].base = tamCodigo;
            mv->tabla_de_segmentos[1].tam = TamRam - tamCodigo;

            mv->tabla_de_registros[CS] = 0x00000000;
            mv->tabla_de_registros[DS] = 0x00010000;
            mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS];
          }
      }
      fclose(Arch);
    }
  
   }


int obtenerDirFisica(MV *mv)
{   int dirFisica;
    dirFisica = mv->tabla_de_segmentos[0].base +(mv->tabla_de_registros[IP] & 0xFFFF);
    return dirFisica;
}
int cantidadOperandos(int codOp)
{
    int cantOper;
    if (codOp >= 0x10)
        cantOper = 2;
    else
        if (codOp <= 0x0A)
            cantOper = 1;
        else
            if (codOp == 0x0F)
                cantOper = 0;
            else
                errorMV("Instruccion invalida");

    return cantOper;
}
void obtenerTiposOperandos(unsigned char instruccion, int cantOper,unsigned char *tipoOpA,unsigned char *tipoOpB)
{   *tipoOpA = 0;
    *tipoOpB = 0;

    if (cantOper == 2)
    {   *tipoOpB = (instruccion >> 6) & 0x03;
        *tipoOpA = (instruccion >> 4) & 0x03;
    }
    else
        if (cantOper == 1)
        *tipoOpA = (instruccion >> 6) & 0x03;
        
}
void cargarOperandos(MV *mv, int dirFisica,int cantOper,unsigned char tipoOpA,unsigned char tipoOpB)
{
    int posOper;
    int i;

    mv->tabla_de_registros[OP1] = 0;
    mv->tabla_de_registros[OP2] = 0;

    posOper = dirFisica + 1;

    if (cantOper == 2)
    {
        mv->tabla_de_registros[OP2] =
            ((long int)tipoOpB << 24);

        for (i = 0; i < tipoOpB; i++)
            mv->tabla_de_registros[OP2] |=((long int)mv->RAM[posOper + i] << (8 * (tipoOpB - 1 - i)));
        posOper += tipoOpB;
    }

    if (cantOper >= 1)
    {
        mv->tabla_de_registros[OP1] =((long int)tipoOpA << 24);
        for (i = 0; i < tipoOpA; i++)
    mv->tabla_de_registros[OP1] |= ((long int)mv->RAM[posOper + i] << (8 * (tipoOpA - 1 - i)));
        
    }
}
int obtenerDirFisicaOperando(MV *mv, long int operando, long int *dirLogicaEfectiva)
{   unsigned char codReg;
    int offsetFinal, dirFisica;
    long int dirLogica ;
    short int desplazamiento;
    unsigned short int segmento,offset;

    codReg = operando & 0x1F;
    desplazamiento = (short int)((operando >> 8) & 0xFFFF);

    dirLogica = mv->tabla_de_registros[codReg];
    segmento = (dirLogica >> 16) & 0xFFFF;
    offset = dirLogica & 0xFFFF;

    offsetFinal = (int)offset + desplazamiento;
    if (segmento >=cantSeg)  // por si es mayor a 7, la tabla solo tiene 8 posiciones
        errorMV("Segmento invalido");

    /*Por si quiere acceder a una pos que ahora no se encuentra habilidada.
    Evaluar si debe permanecer en la 2da parte del TP*/   
    if (mv->tabla_de_segmentos[segmento].base == 0xFFFF && mv->tabla_de_segmentos[segmento].tam == 0xFFFF)
        errorMV("Segmento invalido");

    if (offsetFinal < 0 || offsetFinal+ 4 >= mv->tabla_de_segmentos[segmento].tam)
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
    mv->tabla_de_registros[MAR] = ((long int)4 << 16) |  (dirFisica & 0xFFFF);

    *valor = 0;

    *valor |= ((long int)mv->RAM[dirFisica] << 24);
    *valor |= ((long int)mv->RAM[dirFisica + 1] << 16);
    *valor |= ((long int)mv->RAM[dirFisica + 2] << 8);
    *valor |= ((long int)mv->RAM[dirFisica + 3]);
     mv->tabla_de_registros[MBR] = *valor;
}

/* obtengo valor de op1 / op2*/
long int obtenerValorOperando(MV *mv, long int operando, long int *valor)
{   unsigned char tipo, codReg;
   
    tipo = (operando >> 24) & 0xFF;
 
    if (tipo == 1)//REGISTRO
    {   codReg = operando & 0x1F;
        *valor = mv->tabla_de_registros[codReg];
    }
    else 
      if (tipo == 2)//INMEDIATO
      { *valor = (short int)(operando & 0xFFFF);
      }
      else 
         if (tipo == 3)//MEMORIA
        {  leerMemoria(mv, operando, valor);}
        else
             errorMV("Tipo de operando erroneo");
}
void escribirMemoria(MV *mv, long int operando, long int valor)
{
    int dirFisica;
    long int dirLogica;

    dirFisica = obtenerDirFisicaOperando(mv, operando, &dirLogica);

    mv->tabla_de_registros[LAR] = dirLogica;

    mv->tabla_de_registros[MAR] = ((long int)4 << 16) | (dirFisica & 0xFFFF);

    mv->RAM[dirFisica] = (valor >> 24)& 0xFF;
    mv->RAM[dirFisica +1] = (valor >> 16) & 0xFF;
    mv->RAM[dirFisica +2] = (valor >> 8) & 0xFF;
    mv->RAM[dirFisica +3] = valor & 0xFF;

    mv->tabla_de_registros[MBR] = valor;
}
void guardarValorOperando(MV *mv, long int operando, long int valor)
{  unsigned char tipo,codReg;
   tipo = (operando >> 24) & 0xFF;
   if (tipo == 1)
    {   codReg = operando & 0x1F;
        mv->tabla_de_registros[codReg] = valor;
    }
    else 
       if (tipo == 3)
         escribirMemoria(mv, operando, valor);
        else
        errorMV("Operando destino invalido");
}




void ejecutarInstruccion(MV *mv)
{   if (mv->tabla_de_registros[OPC] == 0x0F)
      mv->tabla_de_registros[IP] = 0xFFFFFFFF;
}
void ejecutarPrograma(MV *mv)
{
    int dirFisica, cantOper,tamInstr;
    unsigned char tipoOpA, tipoOpB,instruccion;

    while (mv->tabla_de_registros[IP] != 0xFFFFFFFF &&(mv->tabla_de_registros[IP] & 0xFFFF) < mv->tabla_de_segmentos[0].tam)
    {
        dirFisica = obtenerDirFisica(mv);
        instruccion = mv->RAM[dirFisica];
        mv->tabla_de_registros[OPC] = instruccion & 0x1F;
        cantOper = cantidadOperandos(mv->tabla_de_registros[OPC]);
        obtenerTiposOperandos(instruccion,cantOper,&tipoOpA, &tipoOpB);

       tamInstr = 1 +tipoOpA+tipoOpB;

        cargarOperandos(mv,dirFisica,cantOper,tipoOpA, tipoOpB);
        mv->tabla_de_registros[IP] += tamInstr;
        ejecutarInstruccion(mv);
    }
}

