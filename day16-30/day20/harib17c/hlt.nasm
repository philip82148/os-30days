[BITS 32]
  MOV  AL, 'A'
  CALL 0xc74
fin:
  HLT
  JMP fin
