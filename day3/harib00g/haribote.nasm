; haribote-os
    ORG 0xc200

    MOV AL, 0x13 ; VGAグラフィックス320x200x8bitカラー
    MOV AH, 0x00
    INT 0x10

fin:
    hlt
    jmp fin
