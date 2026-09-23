#include <stdio.h>
#include <stdlib.h>
#include "MV.h"
#include "Funciones.h"

int main(int argc, char *argv[])
{   MV maquina;
    inicializarMV(&maquina);

    if (argc > 1)
        cargarPrograma(&maquina, argv[1], argc > 2 ? argv[2] : NULL);
    else
        errorMV("Falta indicar el archivo VMX");

    return 0;
}

/*
    argv[1];         //código traducido
    argv[2];         // flag, si no es null se muestra el disassembler
*/