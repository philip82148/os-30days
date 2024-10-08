/*
 * console.c
 */

#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal) {
  struct TASK *task = task_now();
  int fifobuf[128];
  fifo32_init(&task->fifo, 128, fifobuf, task);

  struct TIMER *timer = timer_alloc();
  timer_init(timer, &task->fifo, 1);
  timer_settime(timer, 50);
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
  file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

  // Display prompt
  struct CONSOLE cons;
  cons.sht = sheet;
  cons.cur_x = 8;
  cons.cur_y = 28;
  cons.cur_c = -1;
  *((int *)0x0fec) = (int)&cons;
  cons_putchar(&cons, '>', 1);

  char cmdline[30];
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
          if (cons.cur_c >= 0) cons.cur_c = COL8_FFFFFF;
        } else {
          timer_init(timer, &task->fifo, 1);
          if (cons.cur_c >= 0) cons.cur_c = COL8_000000;
        }
        timer_settime(timer, 50);
      }
      if (data == 2) cons.cur_c = COL8_FFFFFF;  // Cursor ON
      if (data == 3) {                          // Cursor OFF
        boxfill8(
            sheet->buf,
            sheet->bxsize,
            COL8_000000,
            cons.cur_x,
            cons.cur_y,
            cons.cur_x + 7,
            cons.cur_y + 15
        );
        cons.cur_c = -1;
      }
      if (256 <= data && data <= 511) {  // Keyboard data from task_a
        data -= 256;
        if (data == 8) {  // Backspace
          if (cons.cur_x > 16) {
            cons_putchar(&cons, ' ', 0);
            cons.cur_x -= 8;
          }
        } else if (data == 10) {  // Enter
          // Remove cursor with space
          cons_putchar(&cons, ' ', 0);
          cmdline[cons.cur_x / 8 - 2] = 0;
          cons_newline(&cons);
          cons_runcmd(cmdline, &cons, fat, memtotal);
          cons_putchar(&cons, '>', 1);
        } else {  // Normal letter
          if (cons.cur_x < 240) {
            cmdline[cons.cur_x / 8 - 2] = data;
            cons_putchar(&cons, data, 1);
          }
        }
      }
      // Show cursor again
      if (cons.cur_c >= 0)
        boxfill8(
            sheet->buf,
            sheet->bxsize,
            cons.cur_c,
            cons.cur_x,
            cons.cur_y,
            cons.cur_x + 7,
            cons.cur_y + 15
        );

      sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
    }
  }
}

void cons_putchar(struct CONSOLE *cons, int chr, char move) {
  char s[2];
  s[0] = chr;
  s[1] = 0;
  if (s[0] == 0x09) {  // Tab
    for (;;) {
      putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
      cons->cur_x += 8;
      if (cons->cur_x == 8 + 240) {
        cons_newline(cons);
      }
      if (((cons->cur_x - 8) & 0x1f) == 0) break;
    }
  } else if (s[0] == 0x0a) {  // New-line
    cons_newline(cons);
  } else if (s[0] == 0x0d) {
  } else {
    putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
    if (move != 0) {
      cons->cur_x += 8;
      if (cons->cur_x == 8 + 240) cons_newline(cons);
    }
  }
}

void cons_newline(struct CONSOLE *cons) {
  struct SHEET *sheet = cons->sht;
  if (cons->cur_y < 28 + 112) {
    cons->cur_y += 16;  // Next line
  } else {              // Scroll
    for (int y = 28; y < 28 + 112; y++) {
      for (int x = 8; x < 8 + 240; x++)
        sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
    }
    for (int y = 28 + 112; y < 28 + 128; y++) {
      for (int x = 8; x < 8 + 240; x++) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
    }
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
  }
  cons->cur_x = 8;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
  if (my_strcmp(cmdline, "mem") == 0) {
    cmd_mem(cons, memtotal);
  } else if (my_strcmp(cmdline, "cls") == 0) {
    cmd_cls(cons);
  } else if (my_strcmp(cmdline, "dir") == 0) {
    cmd_dir(cons);
  } else if (my_strncmp(cmdline, "type ", 5) == 0) {
    cmd_type(cons, fat, cmdline);
  } else if (cmdline[0] != 0) {
    if (cmd_app(cons, fat, cmdline) == 0) {  // Not command, app, empty line
      putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "Bad command.", 12);
      cons_newline(cons);
      cons_newline(cons);
    }
  }
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  char s[30];
  my_sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
  putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
  cons_newline(cons);
  my_sprintf(s, "free %dKB", memman_total(memman) / 1024);
  putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
  cons_newline(cons);
  cons_newline(cons);
}

void cmd_cls(struct CONSOLE *cons) {
  struct SHEET *sheet = cons->sht;
  for (int y = 28; y < 28 + 128; y++) {
    for (int x = 8; x < 8 + 240; x++) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
  }
  sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
  cons->cur_y = 28;
}

void cmd_dir(struct CONSOLE *cons) {
  struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
  char s[30];
  for (int i = 0; i < 224; i++) {
    if (finfo[i].name[0] == 0x00) break;
    if (finfo[i].name[0] != 0xe5) {
      if ((finfo[i].type & 0x18) == 0) {
        my_sprintf(s, "filename.ext   %d", finfo[i].size);
        for (int j = 0; j < 8; j++) s[j] = finfo[i].name[j];
        s[9] = finfo[i].ext[0];
        s[10] = finfo[i].ext[1];
        s[11] = finfo[i].ext[2];
        putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 30);
        cons_newline(cons);
      }
    }
  }
  cons_newline(cons);
}

void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct FILEINFO *finfo =
      file_search(cmdline + 5, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);

  if (finfo != 0) {  // Found the file
    char *p = (char *)memman_alloc_4k(memman, finfo->size);
    file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
    for (int i = 0; i < finfo->size; i++) {
      cons_putchar(cons, p[i], 1);
    }
    memman_free_4k(memman, (int)p, finfo->size);
  } else {  // Didn't find the file
    putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
    cons_newline(cons);
  }
  cons_newline(cons);
}

void cmd_hlt(struct CONSOLE *cons, int *fat) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct FILEINFO *finfo = file_search("HLT.HRB", (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;

  if (finfo != 0) {  // Found the file
    char *p = (char *)memman_alloc_4k(memman, finfo->size);
    file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
    set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER);
    farcall(0, 1003 * 8);
    memman_free_4k(memman, (int)p, finfo->size);
  } else {
    putfonts8_asc_sht(cons->sht, 8, cons->cur_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
    cons_newline(cons);
  }
  cons_newline(cons);
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;

  char name[18];
  int i;
  // Make filename from command
  for (i = 0; i < 13; i++) {
    if (cmdline[i] <= ' ') break;
    name[i] = cmdline[i];
  }
  name[i] = 0;

  // Look for file
  struct FILEINFO *finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
  // Not found, add ".HRB" and search again
  if (finfo == 0 && name[i - 1] != '.') {
    name[i] = '.';
    name[i + 1] = 'H';
    name[i + 2] = 'R';
    name[i + 3] = 'B';
    name[i + 4] = 0;
    finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
  }

  if (finfo != 0) {  // Found the file
    char *p = (char *)memman_alloc_4k(memman, finfo->size);
    file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
    set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER);
    farcall(0, 1003 * 8);
    memman_free_4k(memman, (int)p, finfo->size);
    cons_newline(cons);
    return 1;
  }

  // Not found
  return 0;
}
