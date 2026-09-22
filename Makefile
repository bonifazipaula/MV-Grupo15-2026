vmx: main.c MV.c Funciones.c Disassembler.c
	gcc -g *.c -o vmx

clean:
	rm -f vmx