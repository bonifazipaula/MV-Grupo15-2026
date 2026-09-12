#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MV.h"
#include "Funciones.h"
#include "Disassembler.h"
#include "Registros.h"

void inicializarMV(MV *mv)
{   int i;
    memset(mv->RAM, 0, sizeof(mv->RAM));
    for (i = 0; i < cantReg; i++)
    {   
        mv->tabla_de_registros[i] = 0;
    }

    for (i = 0; i < cantSeg; i++)
    {
       mv->tabla_de_segmentos[i].base = 0xFFFF;
       mv->tabla_de_segmentos[i].tam = 0xFFFF;
    }
}
void cargarPrograma(MV *mv, char *nombreArchivo)
{ FILE *Arch;
  char identificador[6];
  unsigned char version;
  unsigned short int tamCodigo;

  Arch=fopen(nombreArchivo, "rb");
  if (Arch == NULL)
     errorMV("No se pudo abrir el archivo");
  else
  {  fread(identificador,sizeof(char),5,Arch);
     identificador[5] = '\0';
    
     if (strcmp(identificador, "VMX26") != 0)
       errorMV("Archivo VMX invalido identificador erroneo");
     else
     {    fread(&version,sizeof(unsigned char),1,Arch);
          if(version!=1)
            errorMV("Version VMX invalida");
          else
          {  fread(&tamCodigo, sizeof(unsigned short int), 1, Arch);  
             if (tamCodigo > TamRam)
                 errorMV("El programa es demasiado grande para la memoria");
             if (fread(mv->RAM, sizeof(unsigned char), tamCodigo, Arch) != tamCodigo)
                 errorMV("No se pudo leer completamente el codigo");

            mv->tabla_de_segmentos[0].base = 0;
            mv->tabla_de_segmentos[0].tam = tamCodigo;

            mv->tabla_de_segmentos[1].base = tamCodigo;
            mv->tabla_de_segmentos[1].tam = TamRam - tamCodigo;

            mv->tabla_de_registros[CS] = 0x00000000;
            mv->tabla_de_registros[DS] = 0x00010000;
            mv->tabla_de_registros[IP] = mv->tabla_de_registros[CS];
          }
      }
    }
  
   }



