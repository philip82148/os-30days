/*
 * graphic.c
 */

#include "bootpack.h"

void init_palette() {
  static unsigned char table_rgb[16 * 3] = {
      0x00, 0x00, 0x00,  //  0:black
      0xff, 0x00, 0x00,  //  1:red
      0x00, 0xff, 0x00,  //  2:green
      0xff, 0xff, 0x00,  //  3:yellow
      0x00, 0x00, 0xff,  //  4:blue
      0xff, 0x00, 0xff,  //  5:purple
      0x00, 0xff, 0xff,  //  6:water blue
      0xff, 0xff, 0xff,  //  7:white
      0xc6, 0xc6, 0xc6,  //  8:gray
      0x84, 0x00, 0x00,  //  9:dark red
      0x00, 0x84, 0x00,  // 10:dark green
      0x84, 0x84, 0x00,  // 11:dark yellow
      0x00, 0x00, 0x84,  // 12:dark blue
      0x84, 0x00, 0x84,  // 13:dark purple
      0x00, 0x84, 0x84,  // 14:dark water blue
      0x84, 0x84, 0x84   // 15:dark gray
  };
  set_palette(0, 15, table_rgb);
}

void set_palette(int start, int end, unsigned char *rgb) {
  int eflags = io_load_eflags();
  io_cli();
  io_out8(0x03c8, start);
  for (int i = start; i <= end; i++) {
    io_out8(0x03c9, rgb[0] / 4);
    io_out8(0x03c9, rgb[1] / 4);
    io_out8(0x03c9, rgb[2] / 4);
    rgb += 3;
  }
  io_store_eflags(eflags);
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
  for (int y = y0; y <= y1; y++) {
    for (int x = x0; x <= x1; x++) {
      vram[y * xsize + x] = c;
    }
  }
}

void init_screen8(unsigned char *vram, int x, int y) {
  boxfill8(vram, x, COL8_008484, 0, 0, x - 1, y - 29);
  boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, x - 1, y - 28);
  boxfill8(vram, x, COL8_FFFFFF, 0, y - 27, x - 1, y - 27);
  boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, x - 1, y - 1);

  boxfill8(vram, x, COL8_FFFFFF, 3, y - 24, 59, y - 24);
  boxfill8(vram, x, COL8_FFFFFF, 2, y - 24, 2, y - 4);
  boxfill8(vram, x, COL8_848484, 3, y - 4, 59, y - 4);
  boxfill8(vram, x, COL8_848484, 59, y - 23, 59, y - 5);
  boxfill8(vram, x, COL8_000000, 2, y - 3, 59, y - 3);
  boxfill8(vram, x, COL8_000000, 60, y - 24, 60, y - 3);

  boxfill8(vram, x, COL8_848484, x - 47, y - 24, x - 4, y - 24);
  boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y - 4);
  boxfill8(vram, x, COL8_FFFFFF, x - 47, y - 3, x - 4, y - 3);
  boxfill8(vram, x, COL8_FFFFFF, x - 3, y - 24, x - 3, y - 3);
}

void putfont8(unsigned char *vram, int xsize, int x, int y, unsigned char c, unsigned char *font) {
  for (int i = 0; i < 16; i++) {
    unsigned char *p = vram + (y + i) * xsize + x;
    unsigned char d = font[i];
    if ((d & 0x80) != 0) p[0] = c;
    if ((d & 0x40) != 0) p[1] = c;
    if ((d & 0x20) != 0) p[2] = c;
    if ((d & 0x10) != 0) p[3] = c;
    if ((d & 0x08) != 0) p[4] = c;
    if ((d & 0x04) != 0) p[5] = c;
    if ((d & 0x02) != 0) p[6] = c;
    if ((d & 0x01) != 0) p[7] = c;
  }
}

void putfonts8_asc(unsigned char *vram, int xsize, int x, int y, unsigned char c, const char *s) {
  extern unsigned char hankaku[4096];
  for (; *s != 0x00; s++) {
    putfont8(vram, xsize, x, y, c, hankaku + ((unsigned char)*s) * 16);
    x += 8;
  }
}

void init_mouse_cursor8(unsigned char *mouse, unsigned char bc) {
  static unsigned char cursor[16][16] = {
      "**************..",
      "*OOOOOOOOOOO*...",
      "*OOOOOOOOOO*....",
      "*OOOOOOOOO*.....",
      "*OOOOOOOO*......",
      "*OOOOOOO*.......",
      "*OOOOOOO*.......",
      "*OOOOOOOO*......",
      "*OOOO**OOO*.....",
      "*OOO*..*OOO*....",
      "*OO*....*OOO*...",
      "*O*......*OOO*..",
      "**........*OOO*.",
      "*..........*OOO*",
      "............*OO*",
      ".............***"
  };

  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      if (cursor[y][x] == '*') {
        mouse[y * 16 + x] = COL8_000000;
      }
      if (cursor[y][x] == 'O') {
        mouse[y * 16 + x] = COL8_FFFFFF;
      }
      if (cursor[y][x] == '.') {
        mouse[y * 16 + x] = bc;
      }
    }
  }
}

void putblock8_8(
    unsigned char *vram,
    int vxsize,
    int pxsize,
    int pysize,
    int px0,
    int py0,
    unsigned char *buf,
    int bxsize
) {
  for (int y = 0; y < pysize; y++) {
    for (int x = 0; x < pxsize; x++) {
      vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
    }
  }
}
