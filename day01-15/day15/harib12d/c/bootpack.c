/*
 * bootpack.c
 */

#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, const char *title);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, const char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

struct TSS32 {
  int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
  int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  int es, cs, ss, ds, fs, gs;
  int ldtr, iomap;
};

void task_b_main();

void HariMain() {
  static char keytable[0x54] = {
      0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,   'Q',
      'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S', 'D', 'F',
      'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
      ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
  };
  char s[40];

  init_gdtidt();
  init_pic();
  io_sti();  // 割り込み許可

  struct FIFO32 fifo;
  int fifobuf[128];
  fifo32_init(&fifo, 128, fifobuf);

  // Init PIT(Timer), Keyboard & mouse
  struct MOUSE_DEC mdec;
  init_pit();
  init_keyboard(&fifo, 256);
  enable_mouse(&fifo, 512, &mdec);

  io_out8(PIC0_IMR, 0xf8);  // Arrow PIT&PIC1&keyboard (11111001)
  io_out8(PIC1_IMR, 0xef);  // Arrow mouse (11101111)

  // Memory
  unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
  memman_free(memman, 0x00400000, memtotal - 0x00400000);

  // Timer
  struct TIMER *timer_ts = timer_alloc();
  timer_init(timer_ts, &fifo, 2);
  timer_settime(timer_ts, 2);
  struct TIMER *timer = timer_alloc();
  timer_init(timer, &fifo, 10);
  timer_settime(timer, 1000);
  struct TIMER *timer2 = timer_alloc();
  timer_init(timer2, &fifo, 3);
  timer_settime(timer2, 300);
  struct TIMER *timer3 = timer_alloc();
  timer_init(timer3, &fifo, 1);
  timer_settime(timer3, 50);

  init_palette();

  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  struct SHTCTL *shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  struct SHEET *sht_back = sheet_alloc(shtctl);
  struct SHEET *sht_mouse = sheet_alloc(shtctl);
  struct SHEET *sht_win = sheet_alloc(shtctl);

  unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
  sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);  // No transparency

  unsigned char buf_mouse[256];
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);  // Transparent Number is 99

  unsigned char *buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
  sheet_setbuf(sht_win, buf_win, 160, 52, -1);  // No transparency

  init_screen8(buf_back, binfo->scrnx, binfo->scrny);
  init_mouse_cursor8(buf_mouse, 99);
  make_window8(buf_win, 160, 68, "window");
  make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
  int cursor_x = 8;
  int cursor_c = COL8_FFFFFF;
  sheet_slide(sht_back, 0, 0);

  // Centering in screen
  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;
  sheet_slide(sht_mouse, mx, my);
  sheet_slide(sht_win, 80, 72);
  sheet_updown(sht_back, 0);
  sheet_updown(sht_win, 1);
  sheet_updown(sht_mouse, 2);
  my_sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
  my_sprintf(s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
  putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
  sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

  struct TSS32 tss_a, tss_b;
  tss_a.ldtr = 0;
  tss_a.iomap = 0x40000000;
  tss_b.ldtr = 0;
  tss_b.iomap = 0x40000000;
  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
  set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);
  set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);
  load_tr(3 * 8);
  int task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
  tss_b.eip = (int)&task_b_main;
  tss_b.eflags = 0x00000202; /* IF = 1; */
  tss_b.eax = 0;
  tss_b.ecx = 0;
  tss_b.edx = 0;
  tss_b.ebx = 0;
  tss_b.esp = task_b_esp;
  tss_b.ebp = 0;
  tss_b.esi = 0;
  tss_b.edi = 0;
  tss_b.es = 1 * 8;
  tss_b.cs = 2 * 8;
  tss_b.ss = 1 * 8;
  tss_b.ds = 1 * 8;
  tss_b.fs = 1 * 8;
  tss_b.gs = 1 * 8;
  *((int *)0x0fec) = (int)sht_back;

  for (;;) {
    io_cli();  // 一旦割り込み禁止
    if (fifo32_status(&fifo) == 0) {
      io_stihlt();  // 割り込み許可
    } else {
      int data = fifo32_get(&fifo);
      io_sti();
      if (data == 2) {
        farjmp(0, 4 * 8);
        timer_settime(timer_ts, 2);
      } else if (data >= 256 && data < 512) {  // Keyboard
        data -= 256;
        my_sprintf(s, "%02X", data);
        putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
        if (data == 0x0e) {  // Backspace
          if (cursor_x > 8) {
            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
            cursor_x -= 8;
          }
        } else if (data < 0x54 && keytable[data] != 0) {
          if (cursor_x < 144) {
            s[0] = keytable[data];
            s[1] = 0;
            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
            cursor_x += 8;
          }
        }
        // Cursor
        boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
        sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
      } else if (data >= 512 && data < 768) {  // Mouse
        data -= 512;
        if (mouse_decode(&mdec, data) != 0) {
          my_sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
          if (mdec.btn & 0x01) s[1] = 'L';
          if (mdec.btn & 0x02) s[3] = 'R';
          if (mdec.btn & 0x04) s[2] = 'C';
          putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
          // Mouse movement
          mx += mdec.x;
          my += mdec.y;

          // Prevent mouse going out of display
          if (mx < 0) mx = 0;
          if (my < 0) my = 0;
          if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
          if (my > binfo->scrny - 1) my = binfo->scrny - 1;

          my_sprintf(s, "(%3d, %3d)", mx, my);
          putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
          sheet_slide(sht_mouse, mx, my);
          if (mdec.btn & 0x01) sheet_slide(sht_win, mx - 80, my - 8);
        }
      } else if (data == 10) {  // 10 sec timer
        putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
      } else if (data == 3) {  // 3 sec timer
        putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
      } else if (data <= 1) {  // Cursor timer
        if (data) {
          timer_init(timer3, &fifo, 0);
          cursor_c = COL8_000000;
        } else {
          timer_init(timer3, &fifo, 1);
          cursor_c = COL8_FFFFFF;
        }
        timer_settime(timer3, 50);
        boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
        sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
      }
    }
  }
}

void make_window8(unsigned char *buf, int xsize, int ysize, const char *title) {
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
      "@@@@@@@@@@@@@@@@"
  };
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
  putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
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
  boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
  putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
  sheet_refresh(sht, x, y, x + l * 8, y + 16);
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

void task_b_main() {
  char s[11];

  struct FIFO32 fifo;
  int fifobuf[128];
  fifo32_init(&fifo, 128, fifobuf);

  struct TIMER *timer_ts = timer_alloc();
  timer_init(timer_ts, &fifo, 1);
  timer_settime(timer_ts, 2);
  struct SHEET *sht_back = (struct SHEET *)*((int *)0x0fec);

  int count = 0;
  for (;;) {
    count++;
    my_sprintf(s, "%10d", count);
    putfonts8_asc_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s, 10);
    io_cli();
    if (fifo32_status(&fifo) == 0) {
      io_sti();
    } else {
      int data = fifo32_get(&fifo);
      io_sti();
      if (data == 1) {
        farjmp(0, 3 * 8);
        timer_settime(timer_ts, 2);
      }
    }
  }
}
