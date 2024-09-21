/*
 * window.c
 */

#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, const char *title, char act) {
  boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
  boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
  boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
  boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
  boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
  boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
  boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
  boxfill8(buf, xsize, COL8_000084, 3, 3, xsize - 4, 20);
  boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
  boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
  make_wtitle8(buf, xsize, title, act);
}

void make_wtitle8(unsigned char *buf, int xsize, const char *title, char act) {
  static char close_btn[14][16] = {
      "OOOOOOOOOOOOOOO@",
      "OQQQQQQQQQQQQQ$@",
      "OQQQQQQQQQQQQQ$@",
      "OQQQ@@QQQQ@@QQ$@",
      "OQQQQ@@QQ@@QQQ$@",
      "OQQQQQ@@@@QQQQ$@",
      "OQQQQQQ@@QQQQQ$@",
      "OQQQQQ@@@@QQQQ$@",
      "OQQQQ@@QQ@@QQQ$@",
      "OQQQ@@QQQQ@@QQ$@",
      "OQQQQQQQQQQQQQ$@",
      "OQQQQQQQQQQQQQ$@",
      "O$$$$$$$$$$$$$$@",
      "@@@@@@@@@@@@@@@@",
  };

  char tc, tbc;
  if (act != 0) {
    tc = COL8_FFFFFF;
    tbc = COL8_000084;
  } else {
    tc = COL8_C6C6C6;
    tbc = COL8_848484;
  }

  boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
  putfonts8_asc(buf, xsize, 24, 4, tc, title);
  for (int y = 0; y < 14; y++) {
    for (int x = 0; x < 16; x++) {
      char color = close_btn[y][x];
      if (color == '@') {
        color = COL8_000000;
      } else if (color == '$') {
        color = COL8_848484;
      } else if (color == 'Q') {
        color = COL8_C6C6C6;
      } else {
        color = COL8_FFFFFF;
      }
      buf[(5 + y) * xsize + (xsize - 21 + x)] = color;
    }
  }
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, const char *s, int l) {
  struct TASK *task = task_now();
  boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
  if (task->langmode != 0 && task->langbyte1 != 0) {
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x - 8, y, x + l * 8, y + 16);
  } else {
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);
  }
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c) {
  int x1 = x0 + sx, y1 = y0 + sy;
  boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
  boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
  boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
  boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
  boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
  boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
  boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
  boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
  boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
}

void change_wtitle8(struct SHEET *sht, char act) {
  char tc_new, tbc_new, tc_old, tbc_old;
  if (act != 0) {
    tc_new = COL8_FFFFFF;
    tbc_new = COL8_000084;
    tc_old = COL8_C6C6C6;
    tbc_old = COL8_848484;
  } else {
    tc_new = COL8_C6C6C6;
    tbc_new = COL8_848484;
    tc_old = COL8_FFFFFF;
    tbc_old = COL8_000084;
  }

  int xsize = sht->bxsize;
  char *buf = sht->buf;
  for (int y = 3; y <= 20; y++) {
    for (int x = 3; x <= xsize - 4; x++) {
      char c = buf[y * xsize + x];
      if (c == tc_old && x <= xsize - 22) {
        c = tc_new;
      } else if (c == tbc_old) {
        c = tbc_new;
      }
      buf[y * xsize + x] = c;
    }
  }
  sheet_refresh(sht, 3, 3, xsize, 21);
}
