; functions

[BITS 32]                   ; 32bitモード用の機械語を作らせる

; オブジェクトファイルのための情報
    GLOBAL io_hlt ; このプログラムに含まれる関数名(プロトタイプ宣言のようなもの)

[SECTION .text]     ; オブジェクトファイルはこれを書いてからプログラムを書く

io_hlt: ; void io_hlt(void);
    HLT
    RET
