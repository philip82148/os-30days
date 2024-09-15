/*
 * mouse.c
 */

#include "bootpack.h"

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE     0xf4

struct FIFO32 *mousefifo;
int mousedata0;

// PS/2マウス割込み
void inthandler2c(int *esp) {
  io_out8(PIC1_OCW2, 0x64);  // PIC1へIRQ-12完了通知
  io_out8(PIC0_OCW2, 0x62);  // PIC2へIRQ-02完了通知
  int data = io_in8(PORT_KEYDAT);
  fifo32_put(mousefifo, data + mousedata0);
}

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec) {
  mousefifo = fifo;
  mousedata0 = data0;

  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
  mdec->phase = 0;
  // Return ACK(0xfa) at success
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
  // ACK(0xfa)
  if (mdec->phase == 0) {
    if (dat == 0xfa) {
      mdec->phase = 1;
    }
    return 0;
  }

  // 1st byte
  if (mdec->phase == 1) {
    // Proper byte
    if ((dat & 0xc8) == 0x08) {
      mdec->buf[0] = dat;
      mdec->phase = 2;
    }
    return 0;
  }

  // 2nd byte
  if (mdec->phase == 2) {
    mdec->buf[1] = dat;
    mdec->phase = 3;
    return 0;
  }

  // 3rd byte
  if (mdec->phase == 3) {
    mdec->buf[2] = dat;
    mdec->phase = 1;
    mdec->btn = mdec->buf[0] & 0x07;
    mdec->x = mdec->buf[1];
    mdec->y = mdec->buf[2];

    if ((mdec->buf[0] & 0x10) != 0) {
      mdec->x |= 0xffffff00;
    }
    if ((mdec->buf[0] & 0x20) != 0) {
      mdec->y |= 0xffffff00;
    }

    mdec->y = -mdec->y;  // マウスのy方向の符号が画面と反対
    return 1;
  }

  return -1;
}
