/*
 * bootpack.c
 */

#include "bootpack.h"

extern struct FIFO8 keyfifo;

void HariMain() {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  char s[40];
  unsigned char mcursor[256], keybuf[32];

  init_gdtidt();
  init_pic();
  io_sti();  // 割り込み許可

  fifo8_init(&keyfifo, 32, keybuf);
  io_out8(PIC0_IMR, 0xf9);  // Arrow PIC1&keyboard (11111001)
  io_out8(PIC1_IMR, 0xef);  // Arrow mouse (11101111)

  init_palette();
  init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  init_mouse_cursor8(mcursor, COL8_008484);
  putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
  my_sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

  for (;;) {
    io_cli();
    if (fifo8_status(&keyfifo) == 0) {
      io_stihlt();
    } else {
      int i = fifo8_get(&keyfifo);
      io_sti();
      my_sprintf(s, "%02X", i);
      boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
      putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
    }
  }
}
