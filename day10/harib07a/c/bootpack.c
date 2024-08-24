/*
 * bootpack.c
 */

#include "bootpack.h"

#define MEMMAN_FREES 4090        // About 32KB
#define MEMMAN_ADDR  0x003c0000  // 0x003c0000

// Information about free memory
struct FREEINFO {
  unsigned int addr, size;
};

// Memory Manager
struct MEMMAN {
  int frees, maxfrees, lostsize, losts;
  struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

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

  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  init_palette();
  init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

  int mx = (binfo->scrnx - 16) / 2;
  int my = (binfo->scrny - 28 - 16) / 2;

  unsigned char mcursor[256];
  init_mouse_cursor8(mcursor, COL8_008484);
  putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

  char s[40];
  my_sprintf(s, "(%3d, %3d)", mx, my);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
  my_sprintf(s, "memory %dMB  free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

  for (;;) {
    io_cli();
    if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
      io_stihlt();
    } else {
      if (fifo8_status(&keyfifo) != 0) {
        int i = fifo8_get(&keyfifo);
        io_sti();
        my_sprintf(s, "%02X", i);
        boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
        putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
      } else if (fifo8_status(&mousefifo) != 0) {
        int i = fifo8_get(&mousefifo);
        io_sti();
        // 3-byte of data chunked
        if (mouse_decode(&mdec, i) != 0) {
          my_sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
          if ((mdec.btn & 0x01) != 0) {
            s[1] = 'L';
          }
          if ((mdec.btn & 0x02) != 0) {
            s[3] = 'R';
          }
          if ((mdec.btn & 0x04) != 0) {
            s[2] = 'C';
          }
          boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
          putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
          // Mouse movement
          boxfill8(
              binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15
          );  // Remove mouse
          mx += mdec.x;
          my += mdec.y;

          // Prevent mouse going out of display
          if (mx < 0) {
            mx = 0;
          }
          if (my < 0) {
            my = 0;
          }
          if (mx > binfo->scrnx - 16) {
            mx = binfo->scrnx - 16;
          }
          if (my > binfo->scrny - 16) {
            my = binfo->scrny - 16;
          }

          my_sprintf(s, "(%3d, %3d)", mx, my);
          boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);       // Delete coordinate
          putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);       // Write coordinate
          putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);  // Draw mouse
        }
      }
    }
  }
}

#define EFLAGS_AC_BIT     0x00040000
#define CR0_CACHE_DISABLE 0x60000000

unsigned int memtest(unsigned int start, unsigned int end) {
  char flg486 = 0;
  unsigned int eflg, cr0, i;

  // Check if 386 or later
  eflg = io_load_eflags();
  eflg |= EFLAGS_AC_BIT;  // AC-bit = 1
  io_store_eflags(eflg);
  eflg = io_load_eflags();

  // On 386, AC-bit automatically go back to 0
  if ((eflg & EFLAGS_AC_BIT) != 0) {
    flg486 = 1;
  }
  eflg &= ~EFLAGS_AC_BIT;  // AC-bit = 0
  io_store_eflags(eflg);

  if (flg486 != 0) {
    cr0 = load_cr0();
    cr0 |= CR0_CACHE_DISABLE;  // Disable cache
    store_cr0(cr0);
  }

  i = memtest_sub(start, end);

  if (flg486 != 0) {
    cr0 = load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE;  // Enable cache
    store_cr0(cr0);
  }

  return i;
}

void memman_init(struct MEMMAN *man) {
  man->frees = 0;     // 空き情報の個数
  man->maxfrees = 0;  // freesの最大値
  man->lostsize = 0;  // 解法に失敗した合計サイズ
  man->losts = 0;     // 解法に失敗した回数
}

// 空きサイズの合計を報告
unsigned int memman_total(struct MEMMAN *man) {
  unsigned int sz = 0;
  for (unsigned int i = 0; i < man->frees; i++) {
    sz += man->free[i].size;
  }
  return sz;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
  for (unsigned int i = 0; i < man->frees; i++) {
    // 十分な広さのメモリ発見
    if (man->free[i].size >= size) {
      unsigned int addr = man->free[i].addr;
      man->free[i].addr += size;
      man->free[i].size -= size;
      // free[i]がなくなったので前へつめる
      if (man->free[i].size == 0) {
        man->frees--;
        for (; i < man->frees; i++) {
          man->free[i] = man->free[i + 1];
        }
      }
      return addr;
    }
  }
  return 0;  // 空き無し
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
  // まとめやすさを考えるとfree[]がaddr順に並んでいる方がいい
  // どこに入れるべきか決める
  int i;
  for (i = 0; i < man->frees; i++) {
    if (man->free[i].addr > addr) {
      break;
    }
  }
  // free[i - 1].addr < addr < free[i].addr

  // 前がある
  if (i > 0) {
    // 前の空き領域にまとめられる
    if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
      man->free[i - 1].size += size;
      // 後ろもある
      if (i < man->frees) {
        // 何と後ろともまとめられる
        if (addr + size == man->free[i].addr) {
          man->free[i - 1].size += man->free[i].size;
          man->frees--;
          for (; i < man->frees; i++) {
            man->free[i] = man->free[i + 1];
          }
        }
      }
      return 0;  // Success
    }
  }

  // 前とはまとめられなかった
  if (i < man->frees) {
    // 後ろがある
    if (addr + size == man->free[i].addr) {
      man->free[i].addr = addr;
      man->free[i].size += size;
      return 0;  // Success
    }
  }

  // 前にも後ろにもまとめられない
  if (man->frees < MEMMAN_FREES) {
    for (int j = man->frees; j > i; j--) {
      man->free[j] = man->free[j - 1];
    }
    man->frees++;
    if (man->maxfrees < man->frees) {
      man->maxfrees = man->frees;
    }
    man->free[i].addr = addr;
    man->free[i].size = size;
    return 0;  // Success
  }

  // 後ろにずらせなかった
  man->losts++;
  man->lostsize += size;

  return -1;  // Failure
}
