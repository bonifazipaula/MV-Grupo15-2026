OUT_DIR = Ejecutables/MV1

vmx: main.c MV.c Funciones.c Disassembler.c
	mkdir -p $(OUT_DIR)
	gcc -g *.c -o $(OUT_DIR)/vmx

clean:
	rm -f $(OUT_DIR)/vmx

#direccion en donde se encuentra el traductor para que sea más fácil testear

#cambiar a la raiz del proyecto o a la carpeta Ejecutables si pinta