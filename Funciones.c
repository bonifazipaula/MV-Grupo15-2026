#include "Funciones.h"
#include "MV.h"
#include "Registros.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>         //forzar usar tipos de 32 bits

void iniciaVectorFunciones(VectorFunciones vecF)
{
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
void actualizarCC(MV *mv, long int resultado, int carry, int overflow)
{   mv->tabla_de_registros[CC] = 0;
    if (resultado & 0x80000000)
        mv->tabla_de_registros[CC] |= 0x80000000;
    if (resultado == 0)
        mv->tabla_de_registros[CC] |= 0x40000000;
    if (carry)
        mv->tabla_de_registros[CC] |= 0x20000000;
    if (overflow)
        mv->tabla_de_registros[CC] |= 0x10000000;
}

void errorMV(char *mensaje)
{
    fprintf(stderr, "Error: %s\n", mensaje);
    exit(EXIT_FAILURE);
}
void obtenerValoresOperandos(MV *mv, long int *valorOp1, long int *valorOp2)
{
    obtenerValorOperando(mv, mv->tabla_de_registros[OP1], valorOp1);
    obtenerValorOperando(mv, mv->tabla_de_registros[OP2], valorOp2);
}

//------------------------------- 2 Operandos --------------------------------------
void MOV(MV *mv){
    long int valor;
    obtenerValorOperando(mv, mv->tabla_de_registros[OP2], &valor);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], valor);

}

void ADD(MV *mv){
    int carry, overflow;
    long int valorOp1, valorOp2, resultado;
    unsigned long long int sumaSinSigno;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);

    sumaSinSigno = (unsigned int)valorOp1 + (unsigned int)valorOp2;
    resultado = valorOp1 + valorOp2;
    resultado = (long int)(unsigned int)sumaSinSigno;

    carry = sumaSinSigno > 0xFFFFFFFF;
    overflow = 0;
    if (valorOp1 > 0 && valorOp2 > 0 && resultado < 0)
        overflow = 1;
    if (valorOp1 < 0 && valorOp2 < 0 && resultado >= 0)
        overflow = 1;

    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void SUB(MV *mv){
    int carry, overflow;
    long int valorOp1, valorOp2, resultado;
    long long int resultadoCompleto;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    
    resultadoCompleto = (long long int)valorOp1 - (long long int)valorOp2;
    carry = 0;
     if (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL)
        carry = 1;

    resultado = (long int)(unsigned int)resultadoCompleto;
    overflow = 0;

    if (valorOp1 >= 0 && valorOp2 < 0 && resultado < 0)
        overflow = 1;
    if (valorOp1 < 0 && valorOp2 > 0 && resultado >= 0)
        overflow = 1;
    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);

}

void MUL(MV *mv){
    int carry, overflow;
    long int valorOp1, valorOp2, resultado;
    long long int resultadoCompleto;

    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultadoCompleto = (long long int)valorOp1 * (long long int)valorOp2;
    carry = 0;
    overflow = 0;

    if (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL)
    {   carry = 1;
        overflow = 1;
    }
    resultado = (long int)(unsigned int)resultadoCompleto;
    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);

}   

void DIV(MV *mv){
    int carry, overflow;
    long int valorOp1, valorOp2, resultado;

    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    if (valorOp2 == 0)
        errorMV("Division por cero");
    resultado = valorOp1 / valorOp2;
    mv->tabla_de_registros[AC] = valorOp1 % valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);

}

void CMP(MV *mv)
{   long int valorOp1, valorOp2, resultado;
    long long int resultadoCompleto;
    int carry, overflow;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultadoCompleto = (long long int)valorOp1 - (long long int)valorOp2;

    carry = 0;
    if (resultadoCompleto > 2147483647LL || resultadoCompleto < -2147483648LL)
        carry = 1;
    resultado = (long int)(unsigned int)resultadoCompleto;
    overflow = 0;

    if (valorOp1 >= 0 && valorOp2 < 0 && resultado < 0)
        overflow = 1;

    if (valorOp1 < 0 && valorOp2 > 0 && resultado >= 0)
        overflow = 1;
    actualizarCC(mv, resultado, carry, overflow);
}

void AND(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void OR(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 | valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
    
}

void XOR(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 ^ valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
    
}

void SWAP(MV *mv){
    long int valorOp1, valorOp2, aux;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    aux = valorOp1;
    valorOp1 = valorOp2;
    valorOp2 = aux;
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], valorOp1);
    guardarValorOperando(mv, mv->tabla_de_registros[OP2], valorOp2);
    actualizarCC(mv, valorOp1, 0, 0);   
}

void SHL(MV *mv){
    long int valorOp1, valorOp2, resultado;
    long long int resultadoCompleto;
    int carry, overflow;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultadoCompleto = (long long int)valorOp1 << valorOp2;
   carry = 0;
    overflow = 0;
    if (resultadoCompleto > 2147483647LL ||  resultadoCompleto < -2147483648LL)
    {    carry = 1;
        overflow = 1;
    }
    resultado = (long int)(unsigned int)resultadoCompleto;
    actualizarCC(mv, resultado, carry, overflow);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
    
}

void SHR(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    unsigned int valorSinSigno;
    resultado = valorSinSigno >> valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}
//propaga signo
void SAR(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = valorOp1 >> valorOp2;
    actualizarCC(mv, resultado, 0, 0);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);

}

void LDL(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = (valorOp1 & 0xFFFF0000) | (valorOp2 & 0x0000FFFF);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
}

void LDH(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = ((valorOp2 & 0x0000FFFF) << 16) | (valorOp1 & 0x0000FFFF);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
    
}

void RND(MV *mv){
    long int valorOp1, valorOp2;
    long int resultado;
    obtenerValoresOperandos(mv, &valorOp1, &valorOp2);
    resultado = rand() % (valorOp2 + 1);
    guardarValorOperando(mv, mv->tabla_de_registros[OP1], resultado);
    
}


//------------------------------- 1 Operandos --------------------------------------
void SYS(MV *mv){

}

void JMP(MV *mv){
    
}

void JP(MV *mv){
    
}

void JN(MV *mv){
    
}

void JZ(MV *mv){
    
}

void JC(MV *mv){
    
}

void JV(MV *mv){
    
}
    
void JNP(MV *mv){

}

void JNN(MV *mv){

}

void JNZ(MV *mv){

}

void NOT(MV *mv){

}

//------------------------------- Sin Operandos --------------------------------------
void STOP(MV *mv){
     mv->tabla_de_registros[IP] = 0xFFFFFFFF;
}


