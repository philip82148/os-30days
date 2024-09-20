/*
 * bootpack.c
 */

#include "bootpack.h"

#define KEYCMD_LED 0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);

void HariMain() {
  static char keytable0[0x80] = {
      0,   0,   '1', '2',  '3', '4', '5', '6', '7', '8', '9', '0', '-',  '^',  0x08, 0,
      'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0,    'A',  'S',
      'D', 'F', 'G', 'H',  'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z',  'X',  'C',  'V',
      'B', 'N', 'M', ',',  '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,    0,    0,    0,
      0,   0,   0,   0,    0,   0,   0,   '7', '8', '9', '-', '4', '5',  '6',  '+',  '1',
      '2', '3', '0', '.',  0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,
      0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,
      0,   0,   0,   0x5c, 0,   0,   0,   0,   0,   0,   0,   0,   0,    0x5c, 0,    0,
  };
  static char keytable1[0x80] = {
      0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=',  '~', 0x08, 0,
      'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I', 'O',  'P', '`', '{', 0x0a, 0,   'A',  'S',
      'D', 'F', 'G', 'H',  'J', 'K', 'L', '+', '*',  0,   0,   '}', 'Z',  'X', 'C',  'V',
      'B', 'N', 'M', '<',  '>', '?', 0,   '*', 0,    ' ', 0,   0,   0,    0,   0,    0,
      0,   0,   0,   0,    0,   0,   0,   '7', '8',  '9', '-', '4', '5',  '6', '+',  '1',
      '2', '3', '0', '.',  0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,    0,
      0,   0,   0,   0,    0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,    0,
      0,   0,   0,   '_',  0,   0,   0,   0,   0,    0,   0,   0,   0,    '|', 0,    0,
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

  // sht_cons
  unsigned char *buf_cons[2];
  struct SHEET *sht_cons[2];
  struct TASK *task_cons[2];
  int *cons_fifo[2];
  for (int i = 0; i < 2; i++) {
    sht_cons[i] = sheet_alloc(shtctl);
    buf_cons[i] = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons[i], buf_cons[i], 256, 165, -1);
    make_window8(buf_cons[i], 256, 165, "console", 0);
    make_textbox8(sht_cons[i], 8, 28, 240, 128, COL8_000000);
    task_cons[i] = task_alloc();
    task_cons[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
    task_cons[i]->tss.eip = (int)&console_task;
    task_cons[i]->tss.es = 1 * 8;
    task_cons[i]->tss.cs = 2 * 8;
    task_cons[i]->tss.ss = 1 * 8;
    task_cons[i]->tss.ds = 1 * 8;
    task_cons[i]->tss.fs = 1 * 8;
    task_cons[i]->tss.gs = 1 * 8;
    *((int *)(task_cons[i]->tss.esp + 4)) = (int)sht_cons[i];
    *((int *)(task_cons[i]->tss.esp + 8)) = memtotal;
    task_run(task_cons[i], 2, 2);  // level=2, priority=2
    sht_cons[i]->task = task_cons[i];
    sht_cons[i]->flags |= 0x20;  // Has Cursor
    cons_fifo[i] = (int *)memman_alloc_4k(memman, 128 * 4);
    fifo32_init(&task_cons[i]->fifo, 128, cons_fifo[i], task_cons[i]);
  }

  unsigned char buf_mouse[256];
  struct SHEET *sht_mouse = sheet_alloc(shtctl);
  sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);  // Transparent Number is 99
  init_mouse_cursor8(buf_mouse, 99);
  int mx = (binfo->scrnx - 16) / 2;  // 画面中心になるように
  int my = (binfo->scrny - 28 - 16) / 2;

  sheet_slide(sht_back, 0, 0);
  sheet_slide(sht_cons[1], 56, 6);
  sheet_slide(sht_cons[0], 8, 2);
  sheet_slide(sht_mouse, mx, my);

  sheet_updown(sht_back, 0);
  sheet_updown(sht_cons[1], 1);
  sheet_updown(sht_cons[0], 2);
  sheet_updown(sht_mouse, 3);

  int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
  int mmx = -1, mmy = -1, mmx2 = 0;
  int new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
  struct SHEET *sht = 0, *key_win = sht_cons[0];
  keywin_on(key_win);

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
      if (new_mx >= 0) {
        io_sti();
        sheet_slide(sht_mouse, new_mx, new_my);
        new_mx = -1;
      } else if (new_wx != 0x7fffffff) {
        io_sti();
        sheet_slide(sht, new_wx, new_wy);
        new_wx = 0x7fffffff;
      } else {
        task_sleep(task_a);
        io_sti();
      }
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
          if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0))
            s[0] += 0x20;
        }
        if (s[0] != 0) fifo32_put(&key_win->task->fifo, s[0] + 256);  // Normal letter
        if (data == 0x0f) {                                           // Tab
          keywin_off(key_win);
          int j = key_win->height - 1;
          if (j == 0) j = shtctl->top - 1;
          key_win = shtctl->sheets[j];
          keywin_on(key_win);
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
        if (data == 0x3b && key_shift != 0) {  // Shift + F1
          struct TASK *task = key_win->task;
          if (task != 0 && task->tss.ss0 != 0) {
            cons_putstr0(task->cons, "\nBreak(key) :\n");
            io_cli();
            task->tss.eax = (int)&(task->tss.esp0);
            task->tss.eip = (int)asm_end_app;
            io_sti();
          }
        }
        if (data == 0x57 && shtctl->top > 2)
          sheet_updown(shtctl->sheets[1], shtctl->top - 1);  // F11
        if (data == 0xfa) {
          keycmd_wait = -1;
        }  // Keyboard received data
        if (data == 0xfe) {  // Keyboard didn't receive
          wait_KBC_sendready();
          io_out8(PORT_KEYDAT, keycmd_wait);
        }
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
          new_mx = mx;
          new_my = my;

          // 左クリック
          if ((mdec.btn & 0x01) != 0) {
            if (mmx < 0) {
              // 上の下敷きから順番にマウスが押している下敷きを探す
              for (int j = shtctl->top - 1; j > 0; j--) {
                sht = shtctl->sheets[j];
                int x = mx - sht->vx0;
                int y = my - sht->vy0;

                // マウスが乗っているウインドウの判定
                if (x >= 0 && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                  // 透明でない
                  if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                    sheet_updown(sht, shtctl->top - 1);
                    if (sht != key_win) {
                      keywin_off(key_win);
                      key_win = sht;
                      keywin_on(key_win);
                    }
                    // タイトルバーを掴んだ
                    if (x >= 3 && x < sht->bxsize - 3 && y >= 3 && y < 21) {
                      // ウインドウ移動モードにする
                      mmx = mx;
                      mmy = my;
                      mmx2 = sht->vx0;
                      new_wy = sht->vy0;
                    }
                    // 閉じるボタンのクリック
                    if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                      // アプリが作ったウィンドウ
                      if ((sht->flags & 0x10) != 0) {
                        struct TASK *task = sht->task;
                        cons_putstr0(task->cons, "\nBreak(mouse) :\n");
                        io_cli();
                        task->tss.eax = (int)&(task->tss.esp0);
                        task->tss.eip = (int)asm_end_app;
                        io_sti();
                      }
                    }
                    break;
                  }
                }
              }
            } else {
              // ウインドウ移動モード
              int x = mx - mmx;
              int y = my - mmy;
              sheet_slide(sht, (mmx2 + x + 2) & ~3, sht->vy0 + y);
              new_wx = (mmx2 + x + 2) & ~3;
              new_wy = new_wy + y;
              mmy = my;
            }
          } else {
            // 左ボタンを押していない
            mmx = -1;  // 通常モードにする
            if (new_wx != 0x7fffffff) {
              sheet_slide(sht, new_wx, new_wy);
              new_wx = 0x7fffffff;
            }
          }
        }
      }
    }
  }
}

void keywin_off(struct SHEET *key_win) {
  change_wtitle8(key_win, 0);
  if (key_win->flags & 0x20) fifo32_put(&key_win->task->fifo, 3);
}

void keywin_on(struct SHEET *key_win) {
  change_wtitle8(key_win, 1);
  if (key_win->flags & 0x20) fifo32_put(&key_win->task->fifo, 2);
}
