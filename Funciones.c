#include "Funciones.h"
#include "MV.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>


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


//------------------------------- 2 Operandos --------------------------------------
void MOV(MV *mv){

}

void ADD(MV *mv){

}

void SUB(MV *mv){

}

void MUL(MV *mv){

}   

void DIV(MV *mv){

}

void CMP(MV *mv){

}

void AND(MV *mv){

}

void OR(MV *mv){
    
}

void XOR(MV *mv){
    
}

void SWAP(MV *mv){
    
}

void SHL(MV *mv){
    
}

void SHR(MV *mv){
    
}

void SAR(MV *mv){

}

void LDL(MV *mv){

}

void LDH(MV *mv){
    
}

void RND(MV *mv){
    
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

}