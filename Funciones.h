/** Vector de funciones que se pueden ejecutar en la MV */

#ifndef FUNCIONES_H
#define FUNCIONES_H

#include "MV.h"
typedef void (*VectorFunciones[32])(MV *mv);

void iniciaVectorFunciones(VectorFunciones);        //de 00 a 1F

void MOV(MV *mv);
void ADD(MV *mv);
void SUB(MV *mv);
void MUL(MV *mv);   
void DIV(MV *mv);
void CMP(MV *mv);
void AND(MV *mv);
void OR(MV *mv);
void XOR(MV *mv);
void SWAP(MV *mv);
void SHL(MV *mv);
void SHR(MV *mv);
void SAR(MV *mv);
void LDL(MV *mv);
void LDH(MV *mv);
void RND(MV *mv);

void SYS(MV *mv);
void JMP(MV *mv);
void JP(MV *mv);
void JN(MV *mv);
void JZ(MV *mv);
void JC(MV *mv);
void JV(MV *mv);
void JNP(MV *mv);
void JNN(MV *mv);
void JNZ(MV *mv);
void NOT(MV *mv);

void STOP(MV *mv);

void errorMV(char *mensaje);
#endif