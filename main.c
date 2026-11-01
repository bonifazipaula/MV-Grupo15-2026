#include <stdio.h>
#include <stdlib.h>
#include "MV.h"
#include "Funciones.h"

int main(int argc, char *argv[])
{
    MV maquina;
    Parametros params;

    // Parsear argumentos de la línea de comandos
    parsearArgumentos(argc, argv, &params);

    // Inicializar la MV con el tamaño de memoria indicado
    inicializarMV(&maquina, params.tamMemoria);

    // Guardar referencia al archivo VMI para breakpoints
    maquina.archivoVMI = params.archivoVMI;

    if (params.archivoVMX != NULL)
    {
        // Cargar y ejecutar el programa .vmx
        cargarPrograma(&maquina, &params);
        ejecutarPrograma(&maquina, params.flagDisassembler);
    }
    else if (params.archivoVMI != NULL)
    {
        // Cargar imagen .vmi y continuar ejecución
        cargarImagen(&maquina, params.archivoVMI);
        ejecutarPrograma(&maquina, params.flagDisassembler);
    }
    else
    {
        errorMV("Debe indicarse al menos un archivo .vmx o .vmi");
    }

    liberarMV(&maquina);
    return 0;
}
