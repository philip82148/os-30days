; functions

[BITS 32]                   ; 32bitモード用の機械語を作らせる

; オブジェクトファイルのための情報
    GLOBAL io_hlt
    GLOBAL write_mem8

[SECTION .text]     ; オブジェクトファイルはこれを書いてからプログラムを書く

io_hlt: ; void io_hlt(void);
    HLT
    RET

write_mem8: ; void write_mem8(int addr, int data);
    MOV ECX,   [ESP+4] ; [ESP+4]: addr
    MOV AL,    [ESP+8] ; [ESP+8]: data
    MOV [ECX], AL      ; *[ESP+4] = [ESP+8]
    RET
