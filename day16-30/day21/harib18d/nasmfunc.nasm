    ; functions

[BITS 32]                   ; 32bitモード用の機械語を作らせる

    GLOBAL io_hlt, io_cli, io_sti, io_stihlt
    GLOBAL io_in8,  io_in16,  io_in32
    GLOBAL io_out8, io_out16, io_out32
    GLOBAL io_load_eflags, io_store_eflags
    GLOBAL load_gdtr,      load_idtr
    GLOBAL load_cr0,       store_cr0
    GLOBAL load_tr
    GLOBAL asm_inthandler20, asm_inthandler21, asm_inthandler27, asm_inthandler2c
    GLOBAL memtest_sub
    GLOBAL farjmp,         farcall
    GLOBAL asm_hrb_api,    start_app
    EXTERN inthandler21, inthandler20, inthandler27, inthandler2c
    EXTERN hrb_api

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

load_cr0: ; int load_cr0();
    MOV EAX, CR0
    RET

store_cr0: ; void store_cr0(int cr0);
    MOV EAX, [ESP+4]
    MOV CR0, EAX
    RET

load_tr: ; void load_tr(int tr);
    LTR [ESP+4]
    RET

asm_inthandler20: ; void asm_inthandler20();
    PUSH ES
    PUSH DS
    PUSHAD
    MOV  AX,  SS
    CMP  AX,  1*8
    JNE  .from_app
    ; Interuption detected
    MOV  EAX, ESP
    PUSH SS
    PUSH EAX
    MOV  AX,  SS
    MOV  DS,  AX
    MOV  ES,  AX
    CALL inthandler20
    ADD  ESP, 8
    POPAD
    POP  DS
    POP  ES
    IRETD
.from_app:
    MOV  EAX,     1*8
    MOV  DS,      AX
    MOV  ECX,     [0xfe4]
    ADD  ECX,     -8
    MOV  [ECX+4], SS
    MOV  [ECX],   ESP
    MOV  SS,      AX
    MOV  ES,      AX
    MOV  ESP,     ECX
    CALL inthandler20
    POP  ECX
    POP  EAX
    MOV  SS,      AX
    MOV  ESP,     ECX
    POPAD
    POP  DS
    POP  ES
    IRETD

asm_inthandler21: ; void asm_inthandler21();
    PUSH ES
    PUSH DS
    PUSHAD
    MOV  AX, SS
    CMP  AX, 1*8
    JNE  .from_app

    MOV  EAX, ESP
    PUSH SS
    PUSH EAX
    MOV  AX,  SS
    MOV  DS,  AX
    MOV  ES,  AX
    CALL inthandler21
    ADD  ESP, 8
    POPAD
    POP  DS
    POP  ES
    IRETD
.from_app:
    MOV  EAX,     1*8
    MOV  DS,      AX
    MOV  ECX,     [0xfe4]
    ADD  ECX,     -8
    MOV  [ECX+4], SS
    MOV  [ECX],   ESP
    MOV  SS,      AX
    MOV  ES,      AX
    MOV  ESP,     ECX
    CALL inthandler21
    POP  ECX
    POP  EAX
    MOV  SS,      AX
    MOV  ESP,     ECX
    POPAD
    POP  DS
    POP  ES
    IRETD

asm_inthandler27: ; void asm_inthandler27();
    PUSH ES
    PUSH DS
    PUSHAD
    MOV  AX, SS
    CMP  AX, 1*8
    JNE  .from_app

    MOV  EAX, ESP
    PUSH SS
    PUSH EAX
    MOV  AX,  SS
    MOV  DS,  AX
    MOV  ES,  AX
    CALL inthandler27
    ADD  ESP, 8
    POPAD
    POP  DS
    POP  ES
    IRETD
.from_app:
    MOV  EAX,     1*8
    MOV  DS,      AX
    MOV  ECX,     [0xfe4]
    ADD  ECX,     -8
    MOV  [ECX+4], SS
    MOV  [ECX],   ESP
    MOV  SS,      AX
    MOV  ES,      AX
    MOV  ESP,     ECX
    CALL inthandler27
    POP  ECX
    POP  EAX
    MOV  SS,      AX
    MOV  ESP,     ECX
    POPAD
    POP  DS
    POP  ES
    IRETD

asm_inthandler2c: ; void asm_inthandler2c();
    PUSH ES
    PUSH DS
    PUSHAD
    MOV  AX, SS
    CMP  AX, 1*8
    JNE  .from_app

    MOV  EAX, ESP
    PUSH SS
    PUSH EAX
    MOV  AX,  SS
    MOV  DS,  AX
    MOV  ES,  AX
    CALL inthandler2c
    ADD  ESP, 8
    POPAD
    POP  DS
    POP  ES
    IRETD
.from_app:
    MOV  EAX,     1*8
    MOV  DS,      AX
    MOV  ECX,     [0xfe4]
    ADD  ECX,     -8
    MOV  [ECX+4], SS
    MOV  [ECX],   ESP
    MOV  SS,      AX
    MOV  ES,      AX
    MOV  ESP,     ECX
    CALL inthandler2c
    POP  ECX
    POP  EAX
    MOV  SS,      AX
    MOV  ESP,     ECX
    POPAD
    POP  DS
    POP  ES
    IRETD

memtest_sub: ; unsigned int memtest_sub(unsigned int start, unsigned int end);
    PUSH EDI             ; For (EBX,ESI,EDI)
    PUSH ESI
    PUSH EBX
    MOV  ESI, 0xaa55aa55 ; pat0 = 0xaa55aa55
    MOV  EDI, 0x55aa55aa ; pat1 = 0x55aa55aa
    MOV  EAX, [ESP+12+4] ; i = start
mts_loop:
    MOV EBX,         EAX
    ADD EBX,         0xffc      ; p = i + 0xffc
    MOV EDX,         [EBX]      ; old = *p
    MOV [EBX],       ESI        ; *p = pat0
    XOR DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff
    CMP EDI,         [EBX]      ; if (*p != pat1) goto fin
    JNE mts_fin
    XOR DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff
    CMP ESI,         [EBX]      ; if (*p != pat0) goto fin
    JNE mts_fin
    MOV [EBX],       EDX        ; *p = old
    ADD EAX,         0x1000     ; i += 0x1000
    CMP EAX,         [ESP+12+8] ; if (i <= end) goto mts_loop
    JBE mts_loop
    POP EBX
    POP ESI
    POP EDI
    RET
mts_fin:
    MOV [EBX], EDX ; *p = old
    POP EBX
    POP ESI
    POP EDI
    RET

farjmp: ; void farjmp(int eip, int cs);
    JMP FAR[ESP+4] ; cs:eip
    RET

farcall: ; void farcall(int eip, int cs);
    CALL FAR [ESP+4]
    RET

asm_hrb_api: ; void asm_hrb_api();
    PUSH DS
    PUSH ES
    PUSHAD
    MOV  EAX,      1*8
    MOV  DS,       AX
    MOV  ECX,      [0xfe4]
    ADD  ECX,      -40
    MOV  [ECX+32], ESP
    MOV  [ECX+36], SS

; PUSHADした値値をシステムのスタックにコピーする
    MOV EDX,      [ESP]
    MOV EBX,      [ESP+4]
    MOV [ECX],    EDX
    MOV [ECX+4],  EBX
    MOV EDX,      [ESP+8]
    MOV EBX,      [ESP+12]
    MOV [ECX+8],  EDX
    MOV [ECX+12], EBX
    MOV EDX,      [ESP+16]
    MOV EBX,      [ESP+20]
    MOV [ECX+16], EDX
    MOV [ECX+20], EBX
    MOV EDX,      [ESP+24]
    MOV EBX,      [ESP+28]
    MOV [ECX+24], EDX
    MOV [ECX+28], EBX

    MOV ES,  AX
    MOV SS,  AX
    MOV ESP, ECX
    STI

    CALL hrb_api

    MOV ECX, [ESP+32]
    MOV EAX, [ESP+36]
    CLI
    MOV SS,  AX
    MOV ESP, ECX
    POPAD
    POP ES
    POP DS
    IRETD

start_app:
    PUSHAD
    MOV  EAX,     [ESP+36]
    MOV  ECX,     [ESP+40]
    MOV  EDX,     [ESP+44]
    MOV  EBX,     [ESP+48]
    MOV  [0xfe4], ESP
    CLI
    MOV  ES,      BX
    MOV  SS,      BX
    MOV  DS,      BX
    MOV  FS,      BX
    MOV  GS,      BX
    MOV  ESP,     EDX
    STI
    PUSH ECX
    PUSH EAX
    CALL FAR [ESP]

; アプリが終了するとここに帰ってくる

    MOV EAX, 1*8
    CLI
    MOV ES,  AX
    MOV SS,  AX
    MOV DS,  AX
    MOV FS,  AX
    MOV GS,  AX
    MOV ESP, [0xfe4]
    STI
    POPAD
    RET
