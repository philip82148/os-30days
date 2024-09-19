/*
 * bootpack.c
 */

#include "bootpack.h"

#define KEYCMD_LED 0xed

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
  task_run(task_a, 1, 2);
  *((int *)0x0fe4) = (int)shtctl;

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
        if (data == 0x3b && key_shift != 0 && task_cons->tss.ss0 != 0) {  // Shift + F1
          struct CONSOLE *cons = (struct CONSOLE *)*((int *)0x0fec);
          cons_putstr0(cons, "\nBreak(key) :\n");
          io_cli();
          task_cons->tss.eax = (int)&(task_cons->tss.esp0);
          task_cons->tss.eip = (int)asm_end_app;
          io_sti();
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
