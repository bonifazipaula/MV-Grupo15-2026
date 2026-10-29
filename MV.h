/** Definición de la MV
 *      Nos basamos en la página 2 de /Especificación/TP MV2 2026
 */

#ifndef MV_H
#define MV_H

#include <stdint.h>

#define cantSeg 8       //  si bien en esta segunda parte son 6 se deben inicializar los otros
#define cantReg 32      // en la segunda parte usamos todos
#define TamRamDefault 16384  // 16 KiB por defecto

// Estructura para los parámetros de línea de comandos
typedef struct {
    char *archivoVMX;       // Ruta al archivo .vmx (puede ser NULL)
    char *archivoVMI;       // Ruta al archivo .vmi (puede ser NULL)
    int tamMemoria;         // Tamaño de la memoria en bytes (por defecto 16 KiB)
    int flagDisassembler;   // -d: mostrar código assembler
    int argc;               // Cantidad de parámetros -p (incluyendo el nombre del vmx)
    char **argv;            // Puntero al arreglo de parámetros -p
    int argvStart;          // Índice en argv[] original donde empiezan los params de -p
} Parametros;

typedef struct {
    unsigned short int base;
    unsigned short int tam;
} tabla_segmentos;

//estructura Mv
typedef struct {
    unsigned char *RAM;     // Memoria dinámica
    int tamRAM;             // Tamaño de la RAM en bytes
    tabla_segmentos  tabla_de_segmentos[cantSeg];
    long int tabla_de_registros[cantReg];
    char *archivoVMI;       // Ruta al archivo .vmi (para breakpoints)
    int stepMode;           // 1 = paso a paso (breakpoint en cada instrucción)
} MV;
 
/*funciones de la maquina virtual*/

void inicializarMV(MV *mv, int tamMemoria);
void liberarMV(MV *mv);
void parsearArgumentos(int argc, char *argv[], Parametros *params);
void cargarPrograma(MV *mv, Parametros *params);
void cargarImagen(MV *mv, char *archivoVMI);
void guardarImagen(MV *mv, char *archivoVMI);
void ejecutarPrograma(MV *mv, int flagDisassembler);
void obtenerValorOperando(MV *mv, uint32_t operando, int32_t *valor);
void guardarValorOperando(MV *mv, long int operando, long int valor);
#endif