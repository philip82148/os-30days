; hello-os

CYLS EQU 10

    ORG 0x7c00

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述
    JMP   entry
    DB    0x90
    DB    "HARIBOTE"    ; ブートセレクタの名前を自由に書いてよい (8Byte)
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
    DB    "HARIBOTE-OS" ; ディスクの名前 (11Byte)
    DB    "FAT12   "    ; フォーマットの名前 (8Byte)
    TIMES 18 DB 0       ; とりあえず18バイト開けておく

; プログラム本体

entry:
; レジスタ初期化
    MOV AX, 0
    MOV SS, AX
    MOV SP, 0x7c00
    MOV DS, AX

; ディスクを読む
    MOV AX, 0x0820
    MOV ES, AX     ;
    MOV BX, 0      ; ES:BX=0x8200
    MOV CH, 0      ; シリンダ
    MOV DH, 0      ; ヘッド
    MOV CL, 2      ; セクタ
readloop:
    MOV SI, 0 ; SI=失敗回数
retry:
    MOV AH, 0x02 ; AH=0x02: ディスク読み込み
    MOV AL, 1    ; 1セクタ
    MOV DL, 0x00 ; Aドライブ
    INT 0x13     ; ディスクBIOS呼び出し
    JNC next     ; if no error: goto next
    ADD SI, 1
    CMP SI, 5
    JAE error    ; if SI >= 5: goto error
    MOV AH, 0x00 ; エラーコード消去
    MOV DL, 0x00 ;Aドライブ
    INT 0x13     ;ドライブのリセット
    JMP retry
next:
    MOV AX, ES
    ADD AX, 0x20
    MOV ES, AX   ; ES+=0x20
    ADD CL, 1
    CMP CL, 18
    JBE readloop ; if CL <= 18: goto readloop
    MOV CL, 1
    ADD DH, 1
    CMP DH, 2
    JB  readloop ; if DH < 2: goto readloop
    MOV DH, 0
    ADD CH, 1
    CMP CH, CYLS
    JB  readloop ; if CH < CYLS: goto readloop

    MOV [0x0ff0], CH
    JMP 0xc200       ; jump to the os program

error:
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
    DB 0x0a, 0x0a   ; "\n\n"
    DB "load error"
    DB 0x0a         ; "\n"
    DB 0

    TIMES 0x1fe-($-$$) DB 0
    DB    0x55, 0xaa
