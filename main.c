#include <stdio.h>
#include <stdlib.h>
#include "MV.h"
#include "Funciones.h"
#include "Disassembler.h"
#include "Registros.h"

int main(int argc, char *argv[])
{   MV maquina;
    inicializarMV(&maquina);
    cargarPrograma(&maquina, argv[1]);
    if (argc > 1)
        cargarPrograma(&maquina, argv[1]);
    else
        errorMV("Falta indicar el archivo VMX");

   


    /*
    //vector para poder obtener los argumentos de la linea de comandos
    char* argumento[2];
    printf("holus\n");

    argumento[0]=NULL;
    argumento[1]=NULL;

    argumento[0] = argv[1];         //código traducido
    argumento[1] = argv[2];         // flag, si no es null se muestra el disassembler
    */
    return 0;
}