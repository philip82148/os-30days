; functions

[BITS 32]                   ; 32bitモード用の機械語を作らせる

    GLOBAL io_hlt, io_cli, io_sti, io_stihlt
    GLOBAL io_in8,  io_in16,  io_in32
    GLOBAL io_out8, io_out16, io_out32
    GLOBAL io_load_eflags, io_store_eflags
    GLOBAL load_gdtr,      load_idtr

io_hlt: ; void io_hlt();
    HLT
    RET

io_cli: ; void io_cli();
    CLI
    RET

io_sti: ; void io_sti();
    STI
    RET

io_stihlt: ; void io_stihlt();
    STI
    HLT
    RET

io_in8: ; int io_in8(int port);
    MOV EDX, [ESP+4]
    MOV EAX, 0
    IN  AL,  DX
    RET

io_in16: ; int io_in16(int port);
    MOV EDX, [ESP+4]
    MOV EAX, 0
    IN  AX,  DX
    RET

io_in32: ; int io_in32(int port);
    MOV EDX, [ESP+4]
    IN  EAX, DX
    RET

io_out8: ; void io_out8(int port, int data);
    MOV EDX, [ESP+4]
    MOV AL,  [ESP+8]
    OUT DX,  AL
    RET

io_out16: ; void io_out16(int port, int data);
    MOV EDX, [ESP+4]
    MOV EAX, [ESP+8]
    OUT DX,  AX
    RET

io_out32: ; void io_out32(int port, int data);
    MOV EDX, [ESP+4]
    MOV EAX, [ESP+8]
    OUT DX,  EAX
    RET

io_load_eflags: ; void io_load_eflags();
    PUSHFD
    POP EAX
    RET

io_store_eflags: ; void io_store_eflags(int eflags);
    MOV  EAX, [ESP+4]
    PUSH EAX
    POPFD
    RET

load_gdtr: ; void load_gdtr(int limit, int addr);
        MOV  AX,      [ESP+4]
        MOV  [ESP+6], AX
        LGDT [ESP+6]
        RET

load_idtr: ; void load_idtr(int limit, int addr);
        MOV  AX,      [ESP+4]
        MOV  [ESP+6], AX
        LIDT [ESP+6]
        RET
