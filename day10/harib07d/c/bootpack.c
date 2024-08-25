/*
 * bootpack.c
 */

#include "bootpack.h"

void HariMain() {
  init_gdtidt();
  init_pic();
  io_sti();  // 割り込み許可

  unsigned char keybuf[32], mousebuf[128];
  fifo8_init(&keyfifo, 32, keybuf);
  fifo8_init(&mousefifo, 128, mousebuf);
  io_out8(PIC0_IMR, 0xf9);  // Arrow PIC1&keyboard (11111001)
  io_out8(PIC1_IMR, 0xef);  // Arrow mouse (11101111)

  // Keyboard & mouse
  struct MOUSE_DEC mdec;
  init_keyboard();
  enable_mouse(&mdec);

  // Memory
  unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
  memman_free(memman, 0x00400000, memtotal - 0x00400000);

  init_palette();

  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  struct SHTCTL *shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  struct SHEET *sht_back = sheet_alloc(shtctl);
  struct SHEET *sht_mouse = sheet_alloc(shtctl);

  unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
  sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);  // No transparency

  unsigned char buf_mouse[256];
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);  // Transparent Number is 99

  init_screen8(buf_back, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(buf_mouse, 99);
  sheet_slide(shtctl, sht_back, 0, 0);

  // Centering in screen
  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  sheet_slide(shtctl, sht_mouse, mx, my);
  sheet_updown(shtctl, sht_back, 0);
  sheet_updown(shtctl, sht_mouse, 1);
  char s[40];
  my_sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
  my_sprintf(s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
  sheet_refresh(shtctl);

  for (;;) {
    io_cli();
    if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
      io_stihlt();
    } else {
      if (fifo8_status(&keyfifo) != 0) {
        int i = fifo8_get(&keyfifo);
        io_sti();
        my_sprintf(s, "%02X", i);
        boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
        putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        sheet_refresh(shtctl);
      } else if (fifo8_status(&mousefifo) != 0) {
        int i = fifo8_get(&mousefifo);
        io_sti();
        // 3-byte of data chunked
        if (mouse_decode(&mdec, i) != 0) {
          my_sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
          if ((mdec.btn & 0x01) != 0) s[1] = 'L';
          if ((mdec.btn & 0x02) != 0) s[3] = 'R';
          if ((mdec.btn & 0x04) != 0) s[2] = 'C';
          boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
          putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
          // Mouse movement
          mx += mdec.x;
          my += mdec.y;

          // Prevent mouse going out of display
          if (mx < 0) mx = 0;
          if (my < 0) my = 0;
          if (mx > binfo->scrnx - 16) mx = binfo->scrnx - 16;
          if (my > binfo->scrny - 16) my = binfo->scrny - 16;

          my_sprintf(s, "(%3d, %3d)", mx, my);
          boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);  // Delete coordinate
          putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);  // Write coordinate
          sheet_slide(shtctl, sht_mouse, mx, my);                       // Include sheet_refresh
        }
      }
    }
  }
}
