/*
 * bootpack.c
 */

#include "bootpack.h"

#define KEYCMD_LED 0xed

struct FILEINFO {
  char name[8], ext[3];
  unsigned char type;
  char reserve[10];
  unsigned short time, date, clustno;
  unsigned int size;
};

void make_window8(unsigned char *buf, int xsize, int ysize, const char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, const char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, const char *title, char act);
void console_task(struct SHEET *sheet, unsigned int memtotal);
int cons_newline(int cursor_y, struct SHEET *sheet);
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);

void HariMain() {
  static char keytable0[0x80] = {
      0,   0,   '1', '2',  '3', '4', '5', '6', '7', '8', '9', '0', '-', '^',  0,   0,
      'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,    'A', 'S',
      'D', 'F', 'G', 'H',  'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X',  'C', 'V',
      'B', 'N', 'M', ',',  '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,    0,   0,
      0,   0,   0,   0,    0,   0,   0,   '7', '8', '9', '-', '4', '5', '6',  '+', '1',
      '2', '3', '0', '.',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,
      0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,
      0,   0,   0,   0x5c, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,   0,
  };
  static char keytable1[0x80] = {
      0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
      'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I', 'O',  'P', '`', '{', 0,   0,   'A', 'S',
      'D', 'F', 'G', 'H',  'J', 'K', 'L', '+', '*',  0,   0,   '}', 'Z', 'X', 'C', 'V',
      'B', 'N', 'M', '<',  '>', '?', 0,   '*', 0,    ' ', 0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,    0,   0,   0,   '7', '8',  '9', '-', '4', '5', '6', '+', '1',
      '2', '3', '0', '.',  0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,    0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   '_',  0,   0,   0,   0,   0,    0,   0,   0,   0,   '|', 0,   0,
  };
  char s[40];

  init_gdtidt();
  init_pic();
  io_sti();  // 割り込み許可

  struct FIFO32 fifo;
  int fifobuf[128];
  fifo32_init(&fifo, 128, fifobuf, 0);

  // Init PIT(Timer), Keyboard & mouse
  struct MOUSE_DEC mdec;
  init_pit();
  init_keyboard(&fifo, 256);
  enable_mouse(&fifo, 512, &mdec);

  io_out8(PIC0_IMR, 0xf8);  // Arrow PIT&PIC1&keyboard (11111001)
  io_out8(PIC1_IMR, 0xef);  // Arrow mouse (11101111)
  struct FIFO32 keycmd;
  int keycmd_buf[32];
  fifo32_init(&keycmd, 128, fifobuf, 0);

  // Memory
  unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  memman_init(memman);
  memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
  memman_free(memman, 0x00400000, memtotal - 0x00400000);

  // Display
  init_palette();
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  struct SHTCTL *shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
  struct TASK *task_a = task_init(memman);
  fifo.task = task_a;
  task_run(task_a, 1, 0);

  // sht_back
  struct SHEET *sht_back = sheet_alloc(shtctl);
  unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
  sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);  // 透明色無し
  init_screen8(buf_back, binfo->scrnx, binfo->scrny);

  struct SHEET *sht_cons = sheet_alloc(shtctl);
  unsigned char *buf_cons = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
  sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
  make_window8(buf_cons, 256, 165, "console", 0);
  make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
  struct TASK *task_cons = task_alloc();
  task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
  task_cons->tss.eip = (int)&console_task;
  task_cons->tss.es = 1 * 8;
  task_cons->tss.cs = 2 * 8;
  task_cons->tss.ss = 1 * 8;
  task_cons->tss.ds = 1 * 8;
  task_cons->tss.fs = 1 * 8;
  task_cons->tss.gs = 1 * 8;
  *((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;
  *((int *)(task_cons->tss.esp + 8)) = memtotal;
  task_run(task_cons, 2, 2);  // level=2, priority=2

  // sht_win
  struct SHEET *sht_win = sheet_alloc(shtctl);
  unsigned char *buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
  sheet_setbuf(sht_win, buf_win, 160, 52, -1);  // 透明色無し
  make_window8(buf_win, 160, 68, "task_a", 1);
  make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
  int cursor_x = 8;
  int cursor_c = COL8_FFFFFF;
  struct TIMER *timer = timer_alloc();
  timer_init(timer, &fifo, 1);
  timer_settime(timer, 50);

  unsigned char buf_mouse[256];
  struct SHEET *sht_mouse = sheet_alloc(shtctl);
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);  // Transparent Number is 99
  init_mouse_cursor8(buf_mouse, 99);
  int mx = (binfo->scrnx - 16) / 2;  // 画面中心になるように
  int my = (binfo->scrny - 28 - 16) / 2;

  sheet_slide(sht_back, 0, 0);
  sheet_slide(sht_cons, 32, 4);
  sheet_slide(sht_win, 64, 56);
  sheet_slide(sht_mouse, mx, my);

  sheet_updown(sht_back, 0);
  sheet_updown(sht_cons, 1);
  sheet_updown(sht_win, 2);
  sheet_updown(sht_mouse, 3);

  int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

  // 最初にキーボード状態との食い違いがないように設定する
  fifo32_put(&keycmd, KEYCMD_LED);
  fifo32_put(&keycmd, key_leds);

  for (;;) {
    if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
      // キーボードコントローラに送るデーがあれば送る
      keycmd_wait = fifo32_get(&keycmd);
      wait_KBC_sendready();
      io_out8(PORT_KEYDAT, keycmd_wait);
    }
    io_cli();  // 一旦割り込み禁止
    if (fifo32_status(&fifo) == 0) {
      task_sleep(task_a);
      io_sti();  // 割り込み許可
    } else {
      int data = fifo32_get(&fifo);
      io_sti();
      if (data >= 256 && data < 512) {  // Keyboard
        data -= 256;
        if (data < 0x80) {
          if (key_shift == 0) {
            s[0] = keytable0[data];
          } else {
            s[0] = keytable1[data];
          }
        } else {
          s[0] = 0;
        }
        if ('A' <= s[0] && s[0] <= 'Z') {  // To lowercase
          if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)) {
            s[0] += 0x20;
          }
        }
        if (s[0] != 0) {      // Normal letter
          if (key_to == 0) {  // To task_a
            if (cursor_x < 128) {
              s[1] = 0;
              putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
              cursor_x += 8;
            }
          } else {  // To console
            fifo32_put(&task_cons->fifo, s[0] + 256);
          }
        }
        if (data == 0x0e) {   // Backspace
          if (key_to == 0) {  // To task_a
            if (cursor_x > 8) {
              putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
              cursor_x -= 8;
            }
          } else {
            fifo32_put(&task_cons->fifo, 8 + 256);
          }
        }
        if (data == 0x1c) {   // Enter
          if (key_to != 0) {  // To console
            fifo32_put(&task_cons->fifo, 10 + 256);
          }
        }
        if (data == 0x0f) {  // Tab
          if (key_to == 0) {
            key_to = 1;
            make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
            make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
            cursor_c = -1;  // Hide cursor
            boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
            fifo32_put(&task_cons->fifo, 2);  // Console cursor ON
          } else {
            key_to = 0;
            make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
            make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
            cursor_c = COL8_000000;           // Show cursor
            fifo32_put(&task_cons->fifo, 3);  // Console cursor OFF
          }
          sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
          sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
        }
        if (data == 0x2a) key_shift |= 1;   // Left shift ON
        if (data == 0x36) key_shift |= 2;   // Right shift ON
        if (data == 0xaa) key_shift &= ~1;  // Left shift OFF
        if (data == 0xb6) key_shift &= ~2;  // Right shift OFF
        if (data == 0x3a) {                 // CapsLock
          key_leds ^= 4;
          fifo32_put(&keycmd, KEYCMD_LED);
          fifo32_put(&keycmd, key_leds);
        }
        if (data == 0x45) {  // ScrollLock
          key_leds ^= 1;
          fifo32_put(&keycmd, KEYCMD_LED);
          fifo32_put(&keycmd, key_leds);
        }
        if (data == 0xfa) {
          keycmd_wait = -1;
        }  // Keyboard received data
        if (data == 0xfe) {  // Keyboard didn't receive
          wait_KBC_sendready();
          io_out8(PORT_KEYDAT, keycmd_wait);
        }
        // Show cursor again
        if (cursor_c >= 0)
          boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
        // Cursor re-render
        sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
      } else if (data >= 512 && data < 768) {  // Mouse
        data -= 512;
        if (mouse_decode(&mdec, data) != 0) {
          // Mouse movement
          mx += mdec.x;
          my += mdec.y;

          // Prevent mouse going out of display
          if (mx < 0) mx = 0;
          if (my < 0) my = 0;
          if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
          if (my > binfo->scrny - 1) my = binfo->scrny - 1;

          sheet_slide(sht_mouse, mx, my);
          if (mdec.btn & 0x01) sheet_slide(sht_win, mx - 80, my - 8);
        }
      } else if (data <= 1) {  // Cursor timer
        if (data) {
          timer_init(timer, &fifo, 0);
          if (cursor_c >= 0) cursor_c = COL8_000000;
        } else {
          timer_init(timer, &fifo, 1);
          if (cursor_c >= 0) cursor_c = COL8_FFFFFF;
        }
        timer_settime(timer, 50);
        if (cursor_c >= 0) {
          boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
          sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
        }
      }
    }
  }
}

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

void console_task(struct SHEET *sheet, unsigned int memtotal) {
  struct TASK *task = task_now();
  int fifobuf[128];
  fifo32_init(&task->fifo, 128, fifobuf, task);

  struct TIMER *timer = timer_alloc();
  timer_init(timer, &task->fifo, 1);
  timer_settime(timer, 50);

  // Display prompt
  putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

  char s[30], cmdline[30];
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
  int cursor_x = 16, cursor_y = 28, cursor_c = -1;

  int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
  file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));
  for (;;) {
    io_cli();
    if (fifo32_status(&task->fifo) == 0) {
      task_sleep(task);
      io_sti();
    } else {
      int data = fifo32_get(&task->fifo);
      io_sti();
      if (data <= 1) {  // Timer for cursor
        if (data) {
          timer_init(timer, &task->fifo, 0);
          if (cursor_c >= 0) cursor_c = COL8_FFFFFF;
        } else {
          timer_init(timer, &task->fifo, 1);
          if (cursor_c >= 0) cursor_c = COL8_000000;
        }
        timer_settime(timer, 50);
      }
      if (data == 2) cursor_c = COL8_FFFFFF;  // Cursor ON
      if (data == 3) {                        // Cursor OFF
        boxfill8(
            sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15
        );
        cursor_c = -1;
      }
      if (256 <= data && data <= 511) {  // Keyboard data from task_a
        data -= 256;
        if (data == 8) {  // Backspace
          if (cursor_x > 16) {
            putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
            cursor_x -= 8;
          }
        } else if (data == 10) {  // Enter
          // Remove cursor with space
          putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
          cmdline[cursor_x / 8 - 2] = 0;
          cursor_y = cons_newline(cursor_y, sheet);
          // Execute command
          if (my_strcmp(cmdline, "mem") == 0) {
            my_sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
            cursor_y = cons_newline(cursor_y, sheet);
            my_sprintf(s, "free %dKB", memman_total(memman) / 1024);
            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
            cursor_y = cons_newline(cursor_y, sheet);
            cursor_y = cons_newline(cursor_y, sheet);
          } else if (my_strcmp(cmdline, "cls") == 0) {
            for (int y = 28; y < 28 + 128; y++) {
              for (int x = 8; x < 8 + 240; x++) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
            sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
            cursor_y = 28;
          } else if (my_strcmp(cmdline, "dir") == 0) {
            for (int x = 0; x < 224; x++) {
              if (finfo[x].name[0] == 0x00) break;
              if (finfo[x].name[0] != 0xe5) {
                if ((finfo[x].type & 0x18) == 0) {
                  my_sprintf(s, "filename.ext   %7d", finfo[x].size);
                  for (int y = 0; y < 8; y++) s[y] = finfo[x].name[y];
                  s[9] = finfo[x].ext[0];
                  s[10] = finfo[x].ext[1];
                  s[11] = finfo[x].ext[2];
                  putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                  cursor_y = cons_newline(cursor_y, sheet);
                }
              }
            }
            cursor_y = cons_newline(cursor_y, sheet);
          } else if (my_strncmp(cmdline, "type ", 5) == 0) {
            // Prepare filename
            for (int y = 0; y < 11; y++) s[y] = ' ';
            for (int x = 5, y = 0; y < 11 && cmdline[x] != 0; x++) {
              if (cmdline[x] == '.' && y <= 8) {
                y = 8;
              } else {
                s[y] = cmdline[x];
                // lowercase to uppercase
                if ('a' <= s[y] && s[y] <= 'z') {
                  s[y] -= 0x20;
                }
                y++;
              }
            }
            // Look for the file
            int x;
            for (x = 0; x < 224;) {
              if (finfo[x].name[0] == 0x00) {
                break;
              }
              if ((finfo[x].type & 0x18) == 0) {
                for (int y = 0; y < 11; y++) {
                  if (finfo[x].name[y] != s[y]) {
                    goto type_next_file;
                  }
                }
                break;  // Found the file
              }
            type_next_file:
              x++;
            }
            if (x < 224 && finfo[x].name[0] != 0x00) {  // Found the file
              char *p = (char *)memman_alloc_4k(memman, finfo[x].size);
              file_loadfile(
                  finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00)
              );
              cursor_x = 8;
              for (int y = 0; y < finfo[x].size; y++) {
                s[0] = p[y];
                s[1] = 0;
                if (s[0] == 0x09) {  // tab
                  for (;;) {
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cursor_x += 8;
                    if (cursor_x == 8 + 240) {
                      cursor_x = 8;
                      cursor_y = cons_newline(cursor_y, sheet);
                    }
                    if (((cursor_x - 8) & 0x1f) == 0) break;
                  }
                } else if (s[0] == 0x0a) {  // New-link
                  cursor_x = 8;
                  cursor_y = cons_newline(cursor_y, sheet);
                } else if (s[0] == 0x0d) {  // Return
                                            // Do nothing the moment
                } else {                    // Normal letter
                  putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                  cursor_x += 8;
                  if (cursor_x == 8 + 240) {
                    cursor_x = 8;
                    cursor_y = cons_newline(cursor_y, sheet);
                  }
                }
              }
              memman_free_4k(memman, (int)p, finfo[x].size);
            } else {  // Didn't find the file
              putfonts8_asc_sht(
                  sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15
              );
              cursor_y = cons_newline(cursor_y, sheet);
            }
            cursor_y = cons_newline(cursor_y, sheet);
          } else if (cmdline[0] != 0) {  // Not command, nor empty line
            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command.", 12);
            cursor_y = cons_newline(cursor_y, sheet);
            cursor_y = cons_newline(cursor_y, sheet);
          }
          // Display prompt
          putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
          cursor_x = 16;
        } else {  // Normal letter
          if (cursor_x < 240) {
            s[0] = data;
            s[1] = 0;
            cmdline[cursor_x / 8 - 2] = data;
            putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
            cursor_x += 8;
          }
        }
      }
      // Show cursor again
      if (cursor_c >= 0)
        boxfill8(
            sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15
        );

      sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
    }
  }
}

int cons_newline(int cursor_y, struct SHEET *sheet) {
  if (cursor_y < 28 + 112) {
    cursor_y += 16;  // Next line
  } else {           // Scroll
    for (int y = 28; y < 28 + 112; y++) {
      for (int x = 8; x < 8 + 240; x++)
        sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
    }
    for (int y = 28 + 112; y < 28 + 128; y++) {
      for (int x = 8; x < 8 + 240; x++) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
    }
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
  }
  return cursor_y;
}

void file_readfat(int *fat, unsigned char *img) {
  int i, j = 0;
  for (i = 0; i < 2880; i += 2) {
    fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
    fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
    j += 3;
  }
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
  int i;
  for (;;) {
    if (size <= 512) {
      for (i = 0; i < size; i++) buf[i] = img[clustno * 512 + i];
      break;
    }
    for (i = 0; i < 512; i++) buf[i] = img[clustno * 512 + i];
    size -= 512;
    buf += 512;
    clustno = fat[clustno];
  }
}
