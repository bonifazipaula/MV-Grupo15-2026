#include "Funciones.h"
#include "MV.h"
#include "Registros.h"
#include "Segmentos.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h> 

// Máscaras para el registro CC (N Z C V)
#define MASK_N 0x80000000 // Negativo (bit 31)
#define MASK_Z 0x40000000 // Cero (bit 30)
#define MASK_C 0x20000000 // Acarreo (bit 29)
#define MASK_V 0x10000000 // Desbordamiento (bit 28)


void iniciaVectorFunciones(VectorFunciones vecF) {
    vecF[0x10]=&MOV;
    vecF[0x11]=&ADD;
    vecF[0x12]=&SUB;
    vecF[0x13]=&MUL;
    vecF[0x14]=&DIV;
    vecF[0x15]=&CMP;
    vecF[0x16]=&AND;
    vecF[0x17]=&OR;
    vecF[0x18]=&XOR;
    vecF[0x19]=&SWAP;    
    vecF[0x1A]=&SHL;
    vecF[0x1B]=&SHR;
    vecF[0x1C]=&SAR;
    vecF[0x1D]=&LDL;
    vecF[0x1E]=&LDH;
    vecF[0x1F]=&RND;

    vecF[0x00]=&SYS;
    vecF[0x01]=&JMP;
    vecF[0x02]=&JP;
    vecF[0x03]=&JN;
    vecF[0x04]=&JZ;
    vecF[0x05]=&JC;
    vecF[0x06]=&JV;
    vecF[0x07]=&JNP;
    vecF[0x08]=&JNN;
    vecF[0x09]=&JNZ;    
    vecF[0x0A]=&NOT;

    vecF[0x0F]=&STOP;
}

//---------------------------------Extras---------------------------------------------
void actualizarCC(MV *mv, int32_t resultado, int carry, int overflow)
{   
    mv->tabla_de_registros[CC] = 0;
    if (resultado < 0) // Si el bit más significativo es 1 (N)
        mv->tabla_de_registros[CC] |= MASK_N;
    if (resultado == 0) // Si el resultado es cero (Z)
        mv->tabla_de_registros[CC] |= MASK_Z;
    if (carry) // Acarreo (C)
        mv->tabla_de_registros[CC] |= MASK_C;
    if (overflow) // Desbordamiento (V)
        mv->tabla_de_registros[CC] |= MASK_V;
}

void errorMV(char *mensaje)
{
    fprintf(stderr, "Error: %s\n", mensaje);
    exit(EXIT_FAILURE);
}

void obtenerValoresOperandos(MV *mv, int32_t *valorOp1, int32_t *valorOp2)
{
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], valorOp1);
    obtenerValorOperando(mv, mv->tabla_de_registros[OP2], valorOp2);
}

//------------------------------- 2 Operandos --------------------------------------
void MOV(MV *mv){
    int32_t valor;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP2], &valor);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], valor);
}

void ADD(MV *mv){
    int carry, overflow;
    int32_t valorOp1, valorOp2, resultado;
    uint64_t sumaSinSigno;
    int64_t sumaCompleta;
    
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);

    sumaSinSigno = (uint32_t)valorOp1 + (uint32_t)valorOp2;
    sumaCompleta = (int64_t)valorOp1 + (int64_t)valorOp2;
    resultado = (int32_t)sumaCompleta;

    carry = (sumaSinSigno > 0xFFFFFFFF) ? 1 : 0;
    overflow = (sumaCompleta > 2147483647LL || sumaCompleta < -2147483648LL) ? 1 : 0;

    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void SUB(MV *mv){
    int carry, overflow;
    int32_t valorOp1, valorOp2, resultado;
    int64_t resultadoCompleto;
    
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    
    resultadoCompleto = (int64_t)valorOp1 - (int64_t)valorOp2;
    carry = (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL) ? 1 : 0;
    resultado = (int32_t)resultadoCompleto;
    overflow = 0;

    if (valorOp1 >= 0 && valorOp2 < 0 && resultado < 0) overflow = 1;
    if (valorOp1 < 0 && valorOp2 > 0 && resultado >= 0) overflow = 1;
    
    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void MUL(MV *mv){
    int carry, overflow;
    int32_t valorOp1, valorOp2, resultado;
    int64_t resultadoCompleto;

    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultadoCompleto = (int64_t)valorOp1 * (int64_t)valorOp2;
    
    carry = overflow = (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL) ? 1 : 0;
    resultado = (int32_t)resultadoCompleto;
    
    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}   

void DIV(MV *mv){
    int32_t valorOp1, valorOp2, resultado;

    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    if (valorOp2 == 0) errorMV("Division por cero");
    
    resultado = valorOp1 / valorOp2;
    mv->tabla_de_registros[AC] = (uint32_t)(valorOp1 % valorOp2); // Guarda el resto en AC
    
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void CMP(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    int64_t resultadoCompleto;
    int carry, overflow;
    
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultadoCompleto = (int64_t)valorOp1 - (int64_t)valorOp2;

    carry = (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL) ? 1 : 0;
    resultado = (int32_t)resultadoCompleto;
    overflow = 0;

    if (valorOp1 >= 0 && valorOp2 < 0 && resultado < 0) overflow = 1;
    if (valorOp1 < 0 && valorOp2 > 0 && resultado >= 0) overflow = 1;
    
    actualizarCC(mv, resultado, carry, overflow); // No almacena el resultado, solo setea CC
}

void AND(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 & valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void OR(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 | valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void XOR(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 ^ valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void SWAP(MV *mv){
    int32_t valorOp1, valorOp2, aux;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    aux = valorOp1;
    valorOp1 = valorOp2;
    valorOp2 = aux;
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], valorOp1);
    guardarValorOperando(mv, mv->tabla_de_registros[OP2], valorOp2);
    actualizarCC(mv, valorOp1, 0, 0); // Afecta CC como si fuera el último XOR
}

void SHL(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    int64_t resultadoCompleto;
    int carry, overflow;
    
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultadoCompleto = (int64_t)valorOp1 << valorOp2;
    
    carry = overflow = (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL) ? 1 : 0;
    resultado = (int32_t)resultadoCompleto;
    
    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void SHR(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    uint32_t valorSinSigno = (uint32_t)valorOp1; // Casteo a sin signo para llenar con ceros
    resultado = (int32_t)(valorSinSigno >> valorOp2);
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void SAR(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 >> valorOp2; // Shift aritmético (C propaga el signo en enteros con signo)
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void LDL(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = (valorOp1 & 0xFFFF0000) | (valorOp2 & 0x0000FFFF); // Carga 2 bytes menos significativos
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void LDH(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = ((valorOp2 & 0x0000FFFF) << 16) | (valorOp1 & 0x0000FFFF); // Carga 2 bytes más significativos
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void RND(MV *mv){
    int32_t valorOp1, valorOp2, resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    if (valorOp2 < 0) valorOp2 = 0; // Previene modulo con negativos
    resultado = rand() % (valorOp2 + 1); // Número aleatorio entre 0 y el valor del operando
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

//------------------------------- 1 Operando --------------------------------------

void NOT(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    int32_t resultado = ~valorOp1; // Negación bit a bit
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void JMP(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1; // Efectúa un salto incondicional sumando al CS
}

void JP(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    uint32_t cc = mv->tabla_de_registros[CC];
    if (!(cc & MASK_N) && !(cc & MASK_Z)) // Positivo (> 0)
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void JN(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    if (mv->tabla_de_registros[CC] & MASK_N) // Negativo (< 0)
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void JZ(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    if (mv->tabla_de_registros[CC] & MASK_Z) // Cero (== 0)
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void JC(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    if (mv->tabla_de_registros[CC] & MASK_C) // Acarreo
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void JV(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    if (mv->tabla_de_registros[CC] & MASK_V) // Desbordamiento
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}
    
void JNP(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    uint32_t cc = mv->tabla_de_registros[CC];
    if ((cc & MASK_N) || (cc & MASK_Z)) // Negativo o cero (<= 0)
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void JNN(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    if (!(mv->tabla_de_registros[CC] & MASK_N)) // Positivo o cero (>= 0)
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void JNZ(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    if (!(mv->tabla_de_registros[CC] & MASK_Z)) // Positivo o negativo (!= 0)
        mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS] + valorOp1;
}

void SYS(MV *mv){
    int32_t valorOp1;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], &valorOp1);
    
    uint32_t config = mv->tabla_de_registros[EAX]; // Configuración modo lectura/escritura
    uint32_t cantValores = mv->tabla_de_registros[ECX] & 0x0000FFFF; // Cantidad de valores (2 bytes menos significativos)
    uint32_t tamValores = (mv->tabla_de_registros[ECX] >> 16) & 0x0000FFFF; // Tamaño (2 bytes más significativos)
    uint32_t punteroEDX = mv->tabla_de_registros[EDX]; // Puntero al inicio de la operación de memoria

    if (valorOp1 == 1) {
        // Lógica para SYS 1 (READ)
        for(uint32_t i = 0; i < cantValores; i++) {
            // Calcular dirección física a partir del puntero lógico
            uint32_t dirLogica = punteroEDX + (i * tamValores);
            uint32_t segmento = (dirLogica >> 16) & 0xFFFF;
            uint32_t offset = dirLogica & 0xFFFF;
            int dirFisica = mv->tabla_de_segmentos[segmento].base + offset;

            int32_t datoNuevo = 0;

            // Se utiliza if/else if porque al ingresar por teclado solo se puede tipear en una base a la vez
            // se puede borran los cartelitos pero me parece cómodo para saber que tipo de dato se está ingresando
            if (config & 0x01) {
                printf("Ingrese valor (Decimal): ");
                scanf("%d", &datoNuevo);
            } else if (config & 0x08) {
                printf("Ingrese valor (Hexadecimal): ");
                scanf("%x", &datoNuevo);
            } else if (config & 0x04) {
                printf("Ingrese valor (Octal): ");
                scanf("%o", &datoNuevo);
            } else if (config & 0x02) {
                printf("Ingrese Caracter: ");
                char c;
                scanf(" %c", &c);
                datoNuevo = c;
            } else if (config & 0x10) {
                printf("Ingrese valor (Binario): ");
                char binStr[33];
                scanf("%32s", binStr);
                datoNuevo = (int32_t)strtol(binStr, NULL, 2);
            }

            // Guardar en memoria RAM byte a byte en Big Endian
            for (int b = tamValores - 1; b >= 0; b--) {
                mv->RAM[dirFisica + b] = datoNuevo & 0xFF;
                datoNuevo >>= 8;
            }
        }
    } 
    else if (valorOp1 == 2) {
        // Lógica para SYS 2 (WRITE)
        for(uint32_t i = 0; i < cantValores; i++) {
            // Calcular dirección física
            uint32_t dirLogica = punteroEDX + (i * tamValores);
            uint32_t segmento = (dirLogica >> 16) & 0xFFFF;
            uint32_t offset = dirLogica & 0xFFFF;
            int dirFisica = mv->tabla_de_segmentos[segmento].base + offset;

            int32_t datoLeido = 0;

            // construir los bytes desde la memoria RAM (Big Endian)
            for (uint32_t b = 0; b < tamValores; b++) {
                datoLeido = (datoLeido << 8) | mv->RAM[dirFisica + b];
            }

            // Imprimir evaluando los bits de la máscara (pueden ser múltiples formatos simultáneos, por eso no se usa switch)
            if (config & 0x10) { 
                printf("0b");
                for (int bit = (tamValores * 8) - 1; bit >= 0; bit--) {
                    printf("%d", (datoLeido >> bit) & 1);
                }
                printf(" ");
            }
            if (config & 0x08) { 
                printf("0x%X ", datoLeido);
            }
            if (config & 0x04) { 
                printf("0o%o ", datoLeido);
            }
            if (config & 0x02) { 
                char c = (datoLeido >= 32 && datoLeido <= 126) ? (char)datoLeido : '.';
                printf("%c ", c);
            }
            if (config & 0x01) { 
                printf("%d ", datoLeido);
            }
            printf("\n");
        }
    }
}

//------------------------------- Sin Operandos --------------------------------------
void STOP(MV *mv){
    // Asignar -1 detiene el ciclo de ejecución de la máquina
    mv->tabla_de_registros[IP] = 0xFFFFFFFF;
}