# MV-Grupo15-2026

Implementación en C de la Máquina Virtual MV1, basada en la especificación del
trabajo práctico MV1 2026.

La máquina virtual dispone de memoria RAM, registros y una tabla de segmentos.
Puede cargar un programa binario con formato `.vmx`, validar su encabezado,
ubicarlo en memoria y ejecutar sus instrucciones mediante el conjunto de
operaciones definido para la MV.

## Estructura principal

- `main.c`: punto de entrada del programa y procesamiento del archivo indicado.
- `MV.c` / `MV.h`: memoria virtual, registros, segmentos, carga y ejecución.
- `Funciones.c` / `Funciones.h`: implementación de las instrucciones de la MV.
- `Disassembler.c` / `Disassembler.h`: funcionalidades relacionadas con el disaseembler.
- `Registros.h` y `Segmentos.h`: constantes de registros y segmentos.
- `Makefile`: reglas de compilación y limpieza.
- `Especificación/`: documentación del formato y funcionamiento de la MV.

## Requisitos

- GCC
- GNU Make
- Un sistema Linux o compatible con las herramientas anteriores

## Compilación

Desde la raíz del proyecto, ejecutar:

```bash
make
```

El comando crea el directorio de salida si no existe y genera el ejecutable:

```text
Ejecutables/MV1/vmx
```

También se puede compilar directamente el target del Makefile:

```bash
make vmx
```

Para eliminar el ejecutable generado:

```bash
make clean
```

## Ejecución

El ejecutable recibe como argumento la ruta a un archivo `.vmx`:

```bash
./vmx filename.vmx [-d]
```

Si no se indica un archivo, o si el archivo no tiene un formato VMX válido, la
máquina informa el error correspondiente.