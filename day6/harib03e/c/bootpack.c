/*
 * bootpack.c
 */

#include "bootpack.h"

void HariMain() {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  unsigned char s[40], mcursor[256];

  init_gdtidt();
  init_pic();
  io_sti();  // 割り込み許可

  init_palette();
  init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  init_mouse_cursor8(mcursor, COL8_008484);
  putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
  my_sprintf(s, "(%d, %d)", mx, my);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

  io_out8(PIC0_IMR, 0xf9);  // PIC1&キーボード割り込み許可
  io_out8(PIC1_IMR, 0xef);  // マウス割り込み許可

  for (;;) io_hlt();
}
