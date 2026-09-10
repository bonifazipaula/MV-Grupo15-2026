/** Definición de la MV
 *      Nos basamos en la página 2 de /Especificación/TP MV1 2026
*/

#ifndef MV_H
#define MV_H

#define cantSeg 2       // en esta primera parte son 2 
#define cantReg 32      // en la primer parte usamos 17

typedef struct {
    short int base;
    short int tam;
} tabla_segmentos;

typedef struct {
    unsigned char RAM[16384];
    tabla_segmentos  tabla_de_segmentos[cantSeg];
    long int tabla_de_registros[cantReg];
} MV;


#endif
