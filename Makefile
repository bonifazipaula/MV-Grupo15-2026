OUT_DIR = Ejecutables/MV2
TARGET = $(OUT_DIR)/vmx

$(TARGET): main.c MV.c Funciones.c Disassembler.c
	mkdir -p $(OUT_DIR)
	gcc -g *.c -o $(TARGET)

clean:
	rm -f $(TARGET)

#direccion en donde se encuentra el traductor para que sea más fácil testear

#cambiar a la raiz del proyecto o a la carpeta Ejecutables si pinta