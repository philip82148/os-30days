/*
 * console.c
 */

#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal) {
  struct TASK *task = task_now();

  // Display prompt
  struct CONSOLE cons;
  char cmdline[30];
  cons.sht = sheet;
  cons.cur_x = 8;
  cons.cur_y = 28;
  cons.cur_c = -1;
  task->cons = &cons;
  task->cmdline = cmdline;

  unsigned char *nihongo = (unsigned char *)*((int *)0x0fe8);
  if (nihongo[4096] != 0xff) {
    task->langmode = 1;
  } else {
    task->langmode = 0;
  }
  task->langbyte1 = 0;

  if (cons.sht != 0) {
    cons.timer = timer_alloc();
    timer_init(cons.timer, &task->fifo, 1);
    timer_settime(cons.timer, 50);
  }

  cons_putchar(&cons, '>', 1);

  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
  file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

  struct FILEHANDLE fhandle[8];
  for (int i = 0; i < 8; i++) fhandle[i].buf = 0;  // Not-in-use mark
  task->fhandle = fhandle;
  task->fat = fat;

  for (;;) {
    io_cli();
    if (fifo32_status(&task->fifo) == 0) {
      task_sleep(task);
      io_sti();
    } else {
      int data = fifo32_get(&task->fifo);
      io_sti();
      if (data <= 1 && cons.sht != 0) {  // Timer for cursor
        if (data) {
          timer_init(cons.timer, &task->fifo, 0);
          if (cons.cur_c >= 0) cons.cur_c = COL8_FFFFFF;
        } else {
          timer_init(cons.timer, &task->fifo, 1);
          if (cons.cur_c >= 0) cons.cur_c = COL8_000000;
        }
        timer_settime(cons.timer, 50);
      }
      if (data == 2) cons.cur_c = COL8_FFFFFF;  // Cursor ON
      if (data == 3) {                          // Cursor OFF
        if (cons.sht != 0) {
          boxfill8(
              cons.sht->buf,
              cons.sht->bxsize,
              COL8_000000,
              cons.cur_x,
              cons.cur_y,
              cons.cur_x + 7,
              cons.cur_y + 15
          );
        }
        cons.cur_c = -1;
      }
      if (data == 4) cmd_exit(&cons, fat);
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
          if (cons.sht == 0) cmd_exit(&cons, fat);
          cons_putchar(&cons, '>', 1);
        } else {  // Normal letter
          if (cons.cur_x < 240) {
            cmdline[cons.cur_x / 8 - 2] = data;
            cons_putchar(&cons, data, 1);
          }
        }
      }
      // Show cursor again
      if (cons.sht) {
        if (cons.cur_c >= 0)
          boxfill8(
              cons.sht->buf,
              cons.sht->bxsize,
              cons.cur_c,
              cons.cur_x,
              cons.cur_y,
              cons.cur_x + 7,
              cons.cur_y + 15
          );
        sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
      }
    }
  }
}

void cons_putchar(struct CONSOLE *cons, int chr, char move) {
  char s[2];
  s[0] = chr;
  s[1] = 0;
  if (s[0] == 0x09) {  // Tab
    for (;;) {
      if (cons->sht != 0)
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
    if (cons->sht != 0)
      putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
    if (move != 0) {
      cons->cur_x += 8;
      if (cons->cur_x == 8 + 240) cons_newline(cons);
    }
  }
}

void cons_newline(struct CONSOLE *cons) {
  struct SHEET *sheet = cons->sht;
  struct TASK *task = task_now();
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  int textbox_height = binfo->scrny - 28 - 37;
  if (cons->cur_y < 28 + textbox_height - 16) {
    cons->cur_y += 16;  // Next line
  } else {              // Scroll
    if (sheet) {
      for (int y = 28; y < 28 + textbox_height - 16; y++) {
        for (int x = 8; x < 8 + 240; x++)
          sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
      }
      for (int y = 28 + textbox_height - 16; y < 28 + textbox_height; y++) {
        for (int x = 8; x < 8 + 240; x++) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
      }
      sheet_refresh(sheet, 8, 28, 8 + 240, 28 + textbox_height);
    }
  }
  cons->cur_x = 8;
  if (task->langmode == 1 && task->langbyte1 != 0) cons->cur_x += 8;
}

void cons_putstr0(struct CONSOLE *cons, char *s) {
  for (; *s != 0; s++) cons_putchar(cons, *s, 1);
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l) {
  for (int i = 0; i < l; i++) cons_putchar(cons, s[i], 1);
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
  if (my_strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
    cmd_mem(cons, memtotal);
  } else if (my_strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
    cmd_cls(cons);
  } else if (my_strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
    cmd_dir(cons);
  } else if (my_strcmp(cmdline, "exit") == 0) {
    cmd_exit(cons, fat);
  } else if (my_strncmp(cmdline, "start ", 6) == 0) {
    cmd_start(cons, cmdline, memtotal);
  } else if (my_strncmp(cmdline, "ncst ", 5) == 0) {
    cmd_ncst(cons, cmdline, memtotal);
  } else if (my_strncmp(cmdline, "langmode ", 9) == 0) {
    cmd_langmode(cons, cmdline);
  } else if (cmdline[0] != 0) {
    // Not command, app, empty line
    if (cmd_app(cons, fat, cmdline) == 0) cons_putstr0(cons, "Bad command.\n\n");
  }
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  char s[60];
  my_sprintf(
      s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024
  );
  cons_putstr0(cons, s);
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
        my_sprintf(s, "filename.ext   %d\n", finfo[i].size);
        for (int j = 0; j < 8; j++) s[j] = finfo[i].name[j];
        s[9] = finfo[i].ext[0];
        s[10] = finfo[i].ext[1];
        s[11] = finfo[i].ext[2];
        cons_putstr0(cons, s);
      }
    }
  }
  cons_newline(cons);
}

void cmd_exit(struct CONSOLE *cons, int *fat) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct TASK *task = task_now();
  struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
  struct FIFO32 *fifo = (struct FIFO32 *)*((int *)0x0fec);
  timer_cancel(cons->timer);
  memman_free_4k(memman, (int)fat, 4 * 2880);
  io_cli();
  if (cons->sht != 0) {
    fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);  // 768~1023
  } else {
    fifo32_put(fifo, task - taskctl->tasks0 + 768);  // 1024~2023
  }
  io_sti();
  for (;;) task_sleep(task);
}

void cmd_start(struct CONSOLE *cons, const char *cmdline, int memtotal) {
  struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
  struct SHEET *sht = open_console(shtctl, memtotal);
  struct FIFO32 *fifo = &sht->task->fifo;
  sheet_slide(sht, 32, 4);
  sheet_updown(sht, shtctl->top);
  // Put command letters to a new console
  for (int i = 6; cmdline[i] != 0; i++) fifo32_put(fifo, cmdline[i] + 256);
  fifo32_put(fifo, 10 + 256);  // Enter
  cons_newline(cons);
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal) {
  struct TASK *task = open_constask(0, memtotal);
  struct FIFO32 *fifo = &task->fifo;
  int i;
  // Put command letters to a new console
  for (i = 5; cmdline[i] != 0; i++) {
    fifo32_put(fifo, cmdline[i] + 256);
  }
  fifo32_put(fifo, 10 + 256);
  cons_newline(cons);
}

void cmd_langmode(struct CONSOLE *cons, char *cmdline) {
  struct TASK *task = task_now();
  unsigned char mode = cmdline[9] - '0';
  if (mode <= 2) {
    task->langmode = mode;
  } else {
    cons_putstr0(cons, "mode number error.\n");
  }
  cons_newline(cons);
  return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
  struct TASK *task = task_now();

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
    if (finfo->size >= 36 && my_strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
      int segsiz = *((int *)(p + 0x0000));
      int esp = *((int *)(p + 0x000c));
      int datsiz = *((int *)(p + 0x0010));
      int dathrb = *((int *)(p + 0x0014));
      char *q = (char *)memman_alloc_4k(memman, segsiz);
      task->ds_base = (int)q;
      set_segmdesc(task->ldt + 0, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
      set_segmdesc(task->ldt + 1, segsiz - 1, (int)q, AR_DATA32_RW + 0x60);
      for (i = 0; i < datsiz; i++) q[esp + i] = p[dathrb + i];
      start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
      struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
      for (i = 0; i < MAX_SHEETS; i++) {
        struct SHEET *sht = &(shtctl->sheets0[i]);
        if ((sht->flags & 0x11) == 0x11 && sht->task == task) sheet_free(sht);  // Close
      }
      for (i = 0; i < 8; i++) {  // Close unclosed files
        if (task->fhandle[i].buf != 0) {
          memman_free_4k(memman, (int)task->fhandle[i].buf, task->fhandle[i].size);
          task->fhandle[i].buf = 0;
        }
      }
      timer_cancelall(&task->fifo);
      memman_free_4k(memman, (int)q, segsiz);
      task->langbyte1 = 0;
    } else {
      cons_putstr0(cons, ".hrb file format error.\n");
    }
    memman_free_4k(memman, (int)p, finfo->size);
    cons_newline(cons);
    return 1;
  }

  // Not found
  return 0;
}

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
  struct TASK *task = task_now();
  int ds_base = task->ds_base;
  struct CONSOLE *cons = task->cons;
  struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
  struct FIFO32 *sys_fifo = (struct FIFO32 *)*((int *)0x0fec);
  struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
  int *reg = &eax + 1;  // eaxの次の番地
                        // reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP
                        // reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX

  if (edx == 1) {
    cons_putchar(cons, eax & 0xff, 1);
  } else if (edx == 2) {
    cons_putstr0(cons, (char *)ebx + ds_base);
  } else if (edx == 3) {
    cons_putstr1(cons, (char *)ebx + ds_base, ecx);
  } else if (edx == 4) {
    return &task->tss.esp0;
  } else if (edx == 5) {
    struct SHEET *sht = sheet_alloc(shtctl);
    sht->task = task;
    sht->flags |= 0x10;
    sheet_setbuf(sht, (char *)ebx + ds_base, esi, edi, eax);
    make_window8((char *)ebx + ds_base, esi, edi, (char *)ecx + ds_base, 0);
    sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
    sheet_updown(sht, shtctl->top);
    reg[7] = (int)sht;
  } else if (edx == 6) {
    struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
    putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *)ebp + ds_base);
    if ((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
  } else if (edx == 7) {
    struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
    boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
    if ((ebx & 1) == 0) sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
  } else if (edx == 8) {
    memman_init((struct MEMMAN *)(ebx + ds_base));
    ecx &= 0xfffffff0;  // 16-byte piece
    memman_free((struct MEMMAN *)(ebx + ds_base), eax, ecx);
  } else if (edx == 9) {
    ecx = (ecx + 0x0f) & 0xfffffff0;  // Round up to 16-byte base
    reg[7] = memman_alloc((struct MEMMAN *)(ebx + ds_base), ecx);
  } else if (edx == 10) {
    ecx = (ecx + 0x0f) & 0xfffffff0;  // Round up to 16-byte base
    memman_free((struct MEMMAN *)(ebx + ds_base), eax, ecx);
  } else if (edx == 11) {
    struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
    sht->buf[sht->bxsize * edi + esi] = eax;
    if ((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
  } else if (edx == 12) {
    struct SHEET *sht = (struct SHEET *)ebx;
    sheet_refresh(sht, eax, ecx, esi, edi);
  } else if (edx == 13) {
    struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
    hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
    if ((ebx & 1) == 0) {
      sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
    }
  } else if (edx == 14) {
    sheet_free((struct SHEET *)ebx);
  } else if (edx == 15) {
    for (;;) {
      io_cli();
      if (fifo32_status(&task->fifo) == 0) {
        if (eax != 0) {
          task_sleep(task);  // No FIFO, go to sleep
        } else {
          io_sti();
          reg[7] = -1;
          return 0;
        }
      }
      int data = fifo32_get(&task->fifo);
      io_sti();
      if (data <= 1) {  // Timer for cursor
        timer_init(cons->timer, &task->fifo, 1);
        timer_settime(cons->timer, 50);
      }
      if (data == 2) cons->cur_c = COL8_FFFFFF;  // Cursor ON
      if (data == 3) cons->cur_c = -1;           // Cursor OFF
      if (data == 4) {                           // Close only console
        timer_cancel(cons->timer);
        io_cli();
        fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);  // 2024~2279
        cons->sht = 0;
        io_sti();
      }
      if (data >= 256) {  // Keyboard input
        data -= 256;
        reg[7] = data;
        return 0;
      }
    }
  } else if (edx == 16) {
    reg[7] = (int)timer_alloc();
    ((struct TIMER *)reg[7])->flags2 = 1;  // Auto-cancel ON
  } else if (edx == 17) {
    timer_init((struct TIMER *)ebx, &task->fifo, eax + 256);
  } else if (edx == 18) {
    timer_settime((struct TIMER *)ebx, eax);
  } else if (edx == 19) {
    timer_free((struct TIMER *)ebx);
  } else if (edx == 20) {
    if (eax == 0) {
      int data = io_in8(0x61);
      io_out8(0x61, data & 0x0d);
    } else {
      int data = 1193180000 / eax;
      io_out8(0x43, 0xb6);
      io_out8(0x42, data & 0xff);
      io_out8(0x42, data >> 8);
      data = io_in8(0x61);
      io_out8(0x61, (data | 0x03) & 0x0f);
    }
  } else if (edx == 21) {  // File open
    int i;
    for (i = 0; i < 8; i++) {
      if (task->fhandle[i].buf == 0) break;
    }
    struct FILEHANDLE *fh = &task->fhandle[i];
    reg[7] = 0;
    if (i < 8) {
      struct FILEINFO *finfo =
          file_search((char *)ebx + ds_base, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
      if (finfo != 0) {
        reg[7] = (int)fh;
        fh->buf = (char *)memman_alloc_4k(memman, finfo->size);
        fh->size = finfo->size;
        fh->pos = 0;
        file_loadfile(
            finfo->clustno, finfo->size, fh->buf, task->fat, (char *)(ADR_DISKIMG + 0x003e00)
        );
      }
    }
  } else if (edx == 22) {  // File close
    struct FILEHANDLE *fh = (struct FILEHANDLE *)eax;
    memman_free_4k(memman, (int)fh->buf, fh->size);
    fh->buf = 0;
  } else if (edx == 23) {  // File seek
    struct FILEHANDLE *fh = (struct FILEHANDLE *)eax;
    if (ecx == 0) {
      fh->pos = ebx;
    } else if (ecx == 1) {
      fh->pos += ebx;
    } else if (ecx == 2) {
      fh->pos = fh->size + ebx;
    }

    if (fh->pos < 0) {
      fh->pos = 0;
    }
    if (fh->pos > fh->size) {
      fh->pos = fh->size;
    }
  } else if (edx == 24) {  // Get file size
    struct FILEHANDLE *fh = (struct FILEHANDLE *)eax;
    if (ecx == 0) {
      reg[7] = fh->size;
    } else if (ecx == 1) {
      reg[7] = fh->pos;
    } else if (ecx == 2) {
      reg[7] = fh->pos - fh->size;
    }
  } else if (edx == 25) {  // File read
    struct FILEHANDLE *fh = (struct FILEHANDLE *)eax;
    int i;
    for (i = 0; i < ecx; i++) {
      if (fh->pos == fh->size) {
        break;
      }
      *((char *)ebx + ds_base + i) = fh->buf[fh->pos];
      fh->pos++;
    }
    reg[7] = i;
  } else if (edx == 26) {  // Command line
    int i = 0;
    for (;;) {
      *((char *)ebx + ds_base + i) = task->cmdline[i];
      if (task->cmdline[i] == 0) break;
      if (i >= ecx) break;
      i++;
    }
    reg[7] = i;
  } else if (edx == 27) {  // langmode
    reg[7] = task->langmode;
  }
  return 0;
}

int *inthandler0c(int *esp) {
  struct TASK *task = task_now();
  struct CONSOLE *cons = task->cons;
  char s[30];
  cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
  my_sprintf(s, "EIP = %X\n", esp[11]);
  cons_putstr0(cons, s);
  return &task->tss.esp0;  // Exception halt
}

int *inthandler0d(int *esp) {
  struct TASK *task = task_now();
  struct CONSOLE *cons = task->cons;
  cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
  return &(task->tss.esp0);
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int x = x0 << 10;
  int y = y0 << 10;
  if (dx < 0) dx = -dx;
  if (dy < 0) dy = -dy;

  int len;
  if (dx >= dy) {
    len = dx + 1;

    if (x0 > x1) {
      dx = -1024;
    } else {
      dx = 1024;
    }

    if (y0 <= y1) {
      dy = ((y1 - y0 + 1) << 10) / len;
    } else {
      dy = ((y1 - y0 - 1) << 10) / len;
    }
  } else {
    len = dy + 1;

    if (y0 > y1) {
      dy = -1024;
    } else {
      dy = 1024;
    }

    if (x0 <= x1) {
      dx = ((x1 - x0 + 1) << 10) / len;
    } else {
      dx = ((x1 - x0 - 1) << 10) / len;
    }
  }

  for (int i = 0; i < len; i++) {
    sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
    x += dx;
    y += dy;
  }
}
