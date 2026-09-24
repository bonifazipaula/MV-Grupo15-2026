        MOV EDX, DS
        LDL ECX, 1
        LDH ECX, 4
        MOV EAX, 0x8                                                                                                                                                                                                                                                                        
        SYS 0x1

        MOV ECX, -1	
        MOV EAX, [0]
otro:	CMP EAX, 0
        JZ fin
        SHR EAX, 1
        ADD ECX, 1
        JMP otro

Fin: 	MOV [4], ECX		
        LDL ECX, 2		
        LDH ECX, 4
        MOV EAX, 0x9
        SYS 0x2
        STOP