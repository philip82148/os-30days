/*
 * bootpack.c
 */

#include "bootpack.h"

void HariMain() {
  struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
  unsigned char s[40], mcursor[256];

  init_gdtidt();
  init_palette();
  init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  init_mouse_cursor8(mcursor, COL8_008484);
  putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
  my_sprintf(s, "(%d, %d)", mx, my);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

  for (;;) io_hlt();
}
