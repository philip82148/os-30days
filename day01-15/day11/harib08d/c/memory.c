#include "bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size) {
  size = (size + 0xfff) & 0xfffff000;
  return memman_alloc(man, size);
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
  size = (size + 0xfff) & 0xfffff000;
  return memman_free(man, addr, size);
}
