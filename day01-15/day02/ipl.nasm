; hello-os

    ORG 0x7c00

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述
    JMP   entry
    DB    0x90
    DB    "HELLOIPL"    ; ブートセレクタの名前を自由に書いてよい (8Byte)
    DW    512           ; 1セクタの大きさ (512にしなければならない)
    DB    1             ; クラスタの大きさ (1セクタにしなければならない)
    DW    1             ; FATがどこから始まるか (普通は1セクタ目からにする)
    DB    2             ; FATの個数 (2にしなければならない)
    DW    224           ; ルートディレクトリ領域の大きさ (普通は224エントリにする)
    DW    2880          ; このドライブの大きさ (2880セクタにしなければならない)
    DB    0xf0          ; メディアタイプ (0xf0にしなければならない)
    DW    9             ; FAT領域の長さ (9セクタにしなければならない)
    DW    18            ; 1トラックにいくつのセクタがあるか (18にしなければならない)
    DW    2             ; ヘッドの数 (2にしなければならない)
    DD    0             ; パーティションを使っていないのでここは必ず0
    DD    2880          ; このドライブの大きさをもう一度書く
    DB    0, 0, 0x29    ; よくわからないけどこの値にしておくといいらしい
    DD    0xffffffff    ; たぶんボリュームシリアル番号
    DB    "HELLO-OS   " ; ディスクの名前 (11Byte)
    DB    "FAT12   "    ; フォーマットの名前 (8Byte)
    TIMES 18 DB 0       ; とりあえず18バイト開けておく

; プログラム本体

entry:
; レジスタ初期化
    MOV AX, 0
    MOV SS, AX
    MOV SP, 0x7c00
    MOV DS, AX
    MOV ES, AX

; メッセージ表示
    MOV SI, msg
putloop:
    MOV AL, [SI]
    ADD SI, 1
    CMP AL, 0    ; 終了判定
    JE  fin
    MOV AH, 0x0e ; 一文字表示
    MOV BX, 15   ; カラーコード
    INT 0x10     ; ビデオBIOS呼び出し
    JMP putloop

fin:
    HLT
    JMP fin

msg:
    DB 0x0a, 0x0a     ; "\n\n"
    DB "hello, world"
    DB 0x0a           ; "\n"
    DB 0

    TIMES 0x1fe-($-$$) DB 0
    DB    0x55, 0xaa
