#include "Disassembler.h"
#include "Registros.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

const char* obtenerMnemonico(uint32_t opc) {
    switch(opc) {
        case 0x10: return "MOV"; case 0x11: return "ADD"; case 0x12: return "SUB"; 
        case 0x13: return "MUL"; case 0x14: return "DIV"; case 0x15: return "CMP";
        case 0x16: return "AND"; case 0x17: return "OR";  case 0x18: return "XOR"; 
        case 0x19: return "SWAP";case 0x1A: return "SHL"; case 0x1B: return "SHR";
        case 0x1C: return "SAR"; case 0x1D: return "LDL"; case 0x1E: return "LDH"; 
        case 0x1F: return "RND"; case 0x00: return "SYS"; case 0x01: return "JMP";
        case 0x02: return "JP";  case 0x03: return "JN";  case 0x04: return "JZ";  
        case 0x05: return "JC";  case 0x06: return "JV";  case 0x07: return "JNP";
        case 0x08: return "JNN"; case 0x09: return "JNZ"; case 0x0A: return "NOT"; 
        case 0x0F: return "STOP";
        default: return "???";
    }
}

const char* nombreRegistro(uint32_t codReg) {
    // Mapeo estándar según las posiciones numéricas (10=EAX, 27=DS, etc.)
    static const char* nombres[32] = {
        "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9",
        "EAX", "EBX", "ECX", "EDX", "R14", "R15", "R16", "R17", "R18", "R19",
        "R20", "R21", "R22", "R23", "R24", "R25", "R26", "DS", "CS", "R29", "R30", "R31"
    };
    if (codReg < 32) return nombres[codReg];
    return "???";
}

void formatearOperando(uint32_t operando, char *buffer) {
    uint32_t tipo = (operando >> 24) & 0xFF;

    if (tipo == 0) { 
        buffer[0] = '\0';
    } 
    else if (tipo == 1) { // Registro
        uint32_t codReg = operando & 0x1F;
        sprintf(buffer, "%s", nombreRegistro(codReg));
    } 
    else if (tipo == 2) { // Inmediato
        int16_t inmediato = (int16_t)(operando & 0xFFFF);
        
        // Heurística para mostrar igual a la cátedra ('a', 0x12, 3, etc.)
        if (inmediato >= 32 && inmediato <= 126) {
            sprintf(buffer, "'%c'", (char)inmediato);
        } else if (inmediato > 9 || inmediato < -9) {
            sprintf(buffer, "0x%X", (uint16_t)inmediato);
        } else {
            sprintf(buffer, "%d", inmediato);
        }
    } 
    else if (tipo == 3) { // Memoria
        uint32_t codReg = operando & 0x1F;
        int16_t desp = (int16_t)((operando >> 8) & 0xFFFF);

        if (codReg == 27) { // DS (se omite el registro según la especificación)
            if (desp == 0) sprintf(buffer, "[0]");
            else sprintf(buffer, "[%d]", desp);
        } else {
            if (desp == 0) sprintf(buffer, "[%s]", nombreRegistro(codReg));
            else if (desp > 0) sprintf(buffer, "[%s+%d]", nombreRegistro(codReg), desp);
            else sprintf(buffer, "[%s%d]", nombreRegistro(codReg), desp);
        }
    }
}

/*
aprovecha OPC, OP1 y OP2 para mostrar la instrucción
es necesario de la dirección física para mostrar los bytes de la instrucción en hexa
con el tamaño de la instrucción se puede mostrar la cantidad de bytes que ocupa la instrucción
*/
void muestraInstruccion(MV *mv, int dirFisica, int tamInstr) {
    uint32_t opc = mv->tabla_de_registros[OPC];
    int cantOper = 0;

    if (opc >= 0x10) cantOper = 2;
    else if (opc <= 0x0A) cantOper = 1;

    char op1Str[32] = {0};
    char op2Str[32] = {0};

    if (cantOper >= 1) formatearOperando(mv->tabla_de_registros[OP1], op1Str);
    if (cantOper == 2) formatearOperando(mv->tabla_de_registros[OP2], op2Str);

    char hex[64] = "";
    char byteStr[8];
    for (int i = 0; i < tamInstr; i++) {
        sprintf(byteStr, "%02X ", mv->RAM[dirFisica + i]);
        strcat(hex, byteStr);
    }

    uint32_t dirLogica = mv->tabla_de_registros[IP] & 0xFFFF;
    printf("[%04X] %-22s | %18s", dirLogica, hex, obtenerMnemonico(opc));

    if (cantOper == 1) {
        printf(" %10s", op1Str);
    } else if (cantOper == 2) {
        printf(" %10s, %10s", op1Str, op2Str);
    }
    printf("\n");
}