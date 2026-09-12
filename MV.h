/** Definición de la MV
 *      Nos basamos en la página 2 de /Especificación/TP MV1 2026
*/

#ifndef MV_H
#define MV_H

#define cantSeg 8       //  si bien en esta primera parte son 2 se deben inicializar los otros
#define cantReg 32      // en la primer parte usamos 17
#define TamRam 16384
typedef struct {
    unsigned short int base;
    unsigned short int tam;
} tabla_segmentos;
//estructura Mv
typedef struct {
    unsigned char RAM[TamRam];
    tabla_segmentos  tabla_de_segmentos[cantSeg];
    long int tabla_de_registros[cantReg];
} MV;
 
/*funciones de la maquina virtual*/

void inicializarMV(MV *mv);
void cargarPrograma(MV *mv, char *nombreArchivo);
#endif
