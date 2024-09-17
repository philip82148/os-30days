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
