/*
 * bootpack.c
 */

#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;
void enable_mouse();
void init_keyboard();

void HariMain() {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  char s[40];
  unsigned char mcursor[256], keybuf[32], mousebuf[128];

  init_gdtidt();
  init_pic();
  io_sti();  // 割り込み許可

  fifo8_init(&keyfifo, 32, keybuf);
  fifo8_init(&mousefifo, 128, mousebuf);
  io_out8(PIC0_IMR, 0xf9);  // Arrow PIC1&keyboard (11111001)
  io_out8(PIC1_IMR, 0xef);  // Arrow mouse (11101111)

  init_keyboard();

  init_palette();
  init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  init_mouse_cursor8(mcursor, COL8_008484);
  putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
  my_sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

  enable_mouse();

  for (;;) {
    io_cli();
    if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
      io_stihlt();
    } else {
      if (fifo8_status(&keyfifo) != 0) {
        int i = fifo8_get(&keyfifo);
        io_sti();
        my_sprintf(s, "%02X", i);
        boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
        putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
      } else if (fifo8_status(&mousefifo) != 0) {
        int i = fifo8_get(&mousefifo);
        io_sti();
        my_sprintf(s, "%02X", i);
        boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 47, 31);
        putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
      }
    }
  }
}

#define PORT_KEYDAT          0x0060
#define PORT_KEYSTA          0x0064
#define PORT_KEYCMD          0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE    0x60
#define KBC_MODE             0x47

// Wait for keyboard controller to be ready
void wait_KBC_sendready() {
  for (;;) {
    if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) return;
  }
}

// Reset keyboad controller
void init_keyboard() {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, KBC_MODE);
  return;
}

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE     0xf4

void enable_mouse() {
  wait_KBC_sendready();
  io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
  wait_KBC_sendready();
  io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
  return;  // Return ACK(0xfa) at success
}
