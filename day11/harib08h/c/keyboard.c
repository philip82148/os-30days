/*
 * init.c
 */

#include "bootpack.h"

#define PORT_KEYDAT          0x0060
#define PORT_KEYSTA          0x0064
#define PORT_KEYCMD          0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE    0x60
#define KBC_MODE             0x47

struct FIFO8 keyfifo;

// PS/2キーボード割込み
void inthandler21(int *esp) {
  io_out8(PIC0_OCW2, 0x61);        //  PICへIRQ-01完了通知
  int data = io_in8(PORT_KEYDAT);  // Key code
  fifo8_put(&keyfifo, data);
}

// Wait for keyboard controller to be ready
void wait_KBC_sendready() {
  for (;;) {
    if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) return;
  }
}

// Reset keyboard controller
void init_keyboard() {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);
}
