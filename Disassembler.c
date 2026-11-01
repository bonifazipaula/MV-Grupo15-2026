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
        case 0x0B: return "PUSH"; case 0x0C: return "POP"; case 0x0D: return "CALL";
        case 0x0E: return "RET"; case 0x0F: return "STOP";
        default: return "???";
    }
}

const char* nombreRegistro(uint32_t codReg) {
    static const char* nombres[32] = {
        "IP", "OPC", "OP1", "OP2", "LAR", "MAR", "MBR", "SP", "BP", "",
        "EAX", "EBX", "ECX", "EDX", "EEX", "EFX", "AC", "CC", "", "",
        "", "", "", "", "", "", "CS", "DS", "ES", "SS", "KS", "PS"
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
       uint32_t sector = (operando >> 5) & 0x03;

      if (sector == 0) {
         sprintf(buffer, "%s", nombreRegistro(codReg));
       }
      else if (codReg >= 10 && codReg <= 15) {
        const char *nombres[6][3] = {
           {"AL", "AH", "AX"},
           {"BL", "BH", "BX"},
           {"CL", "CH", "CX"},
           {"DL", "DH", "DX"},
           {"EL", "EH", "EX"},
           {"FL", "FH", "FX"}
       };
        sprintf(buffer, "%s", nombres[codReg - 10][sector - 1]);
}
    else {
        sprintf(buffer, "%s", nombreRegistro(codReg));
        }
    } 
    else if (tipo == 2) { // Inmediato
        int16_t inmediato = (int16_t)(operando & 0xFFFF);
        
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
       uint32_t tamanio = (operando >> 5) & 0x07;
       int16_t desp = (int16_t)((operando >> 8) & 0xFFFF);

       const char *prefijo;

    switch (tamanio) {
        case 1: prefijo = "b"; break;
        case 2: prefijo = "w"; break;
        case 4: prefijo = "l"; break;
        default: prefijo = ""; break;
    }

    if (codReg == 27) {
        sprintf(buffer, "%s[%s%+d]",
        prefijo, nombreRegistro(codReg), desp);
    }
    else if (desp == 0) {
        sprintf(buffer, "%s[%s]", prefijo, nombreRegistro(codReg));
    }
    else if (desp > 0) {
        sprintf(buffer, "%s[%s+%d]", prefijo, nombreRegistro(codReg), desp);
    }
    else {
        sprintf(buffer, "%s[%s%d]", prefijo, nombreRegistro(codReg), desp);
    }
  }
} 


/*
aprovecha OPC, OP1 y OP2 para mostrar la instrucción
es necesario de la dirección física para mostrar los bytes de la instrucción en hexa
con el tamaño de la instrucción se puede mostrar la cantidad de bytes que ocupa la instrucción
*/
void muestraInstruccion(MV *mv, int dirFisica, int tamInstr, int dirEntrada) {
    uint32_t opc = mv->tabla_de_registros[OPC];
    int cantOper = 0;

    if (opc >= 0x10) cantOper = 2;
    else if (opc <= 0x0D) cantOper = 1;

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

    printf("%c[%04X] %-21s | %-8s",
       dirFisica == dirEntrada ? '>' : ' ',
       dirFisica & 0xFFFF,
       hex,
       obtenerMnemonico(opc));

    if (cantOper == 1) {
        printf(" %s", op1Str);
    } else if (cantOper == 2) {
         printf(" %s, %s", op1Str, op2Str);
    }
    printf("\n");
}

void mostrarConstantes(MV *mv) {
    uint32_t ks = (uint32_t)mv->tabla_de_registros[30];
    uint32_t indice = (ks >> 16) & 0xFFFF;

    if (ks == 0xFFFFFFFFu || indice >= cantSeg)
        return;

    uint32_t base = mv->tabla_de_segmentos[indice].base;
    uint32_t tam = mv->tabla_de_segmentos[indice].tam;

    if (base == 0xFFFF || tam == 0 ||
        base >= (uint32_t)mv->tamRAM ||
        tam > (uint32_t)mv->tamRAM - base)
        return;

    uint32_t offset = 0;

    while (offset < tam) {
        uint32_t inicio = offset;

        while (offset < tam && mv->RAM[base + offset] != '\0')
            offset++;

        uint32_t longitud = offset - inicio;
        uint32_t total = longitud + (offset < tam ? 1u : 0u);

        printf(" [%04X] ", (unsigned int)(base + inicio));

        for (uint32_t i = 0; i < 7; i++) {
            if (total > 7 && i == 6) {
                printf(".. ");
            } else if (i < total) {
                printf("%02X ", mv->RAM[base + inicio + i]);
            } else {
                printf("   ");
            }
        }

        printf(" | \"");

        for (uint32_t i = 0; i < longitud; i++) {
            unsigned char c = mv->RAM[base + inicio + i];
            putchar(c >= 32 && c <= 126 ? c : '.');
        }

        printf("\"\n");
        offset += (offset < tam ? 1u : 0u);
    }
}