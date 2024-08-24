/*
 * sheet.c
 */

#include "bootpack.h"

#define SHEET_USE 1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
  struct SHTCTL *ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
  if (ctl == 0) goto err;

  ctl->vram = vram;
  ctl->xsize = xsize;
  ctl->ysize = ysize;
  ctl->top = -1;  // シートは一枚もない

  for (int i = 0; i < MAX_SHEETS; i++) ctl->sheets0[i].flags = 0;  // 未使用マーク

err:
  return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
  for (int i = 0; i < MAX_SHEETS; i++) {
    if (ctl->sheets0[i].flags == 0) {
      struct SHEET *sht = &ctl->sheets0[i];
      sht->flags = SHEET_USE;  // In use
      sht->height = -1;        // Don't display
      return sht;
    }
  }
  return 0;  // All sheets are in use
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
  sht->buf = buf;
  sht->bxsize = xsize;
  sht->bysize = ysize;
  sht->col_inv = col_inv;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height) {
  // ここでのheightはzIndexのこと
  int old_height = sht->height;  // Remember current height

  // 高さの修正
  if (height > ctl->top + 1) height = ctl->top + 1;
  if (height < -1) height = -1;
  sht->height = height;

  // sheets[]の並べ替え
  if (old_height > height) {
    // 以前よりも低くなる
    if (height >= 0) {
      // 間のものを引き上げる
      for (int h = old_height; h > height; h--) {
        ctl->sheets[h] = ctl->sheets[h - 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else {  // 非表示化
      if (ctl->top > old_height) {
        // 上になっているものをおろす
        for (int h = old_height; h < ctl->top; h++) {
          ctl->sheets[h] = ctl->sheets[h + 1];
          ctl->sheets[h]->height = h;
        }
      }
      ctl->top--;  // 憑依中の下敷きが一つ減るので、一番上の高さが減る
    }
    sheet_refresh(ctl);
  } else if (old_height < height) {
    // 以前よりも高くなる
    // 上と同じ
    if (old_height >= 0) {
      for (int h = old_height; h < height; h++) {
        ctl->sheets[h] = ctl->sheets[h + 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else {
      for (int h = ctl->top; h >= height; h--) {
        ctl->sheets[h + 1] = ctl->sheets[h];
        ctl->sheets[h + 1]->height = h + 1;
      }
      ctl->sheets[height] = sht;
      ctl->top++;
    }
    sheet_refresh(ctl);
  }
}

void sheet_refresh(struct SHTCTL *ctl) {
  for (int height = 0; height <= ctl->top; height++) {
    struct SHEET *sht = ctl->sheets[height];
    unsigned char *buf = sht->buf;
    for (int sheet_y = 0; sheet_y < sht->bysize; sheet_y++) {
      int vy = sht->vy0 + sheet_y;
      for (int sheet_x = 0; sheet_x < sht->bxsize; sheet_x++) {
        int vx = sht->vx0 + sheet_x;
        unsigned char color = buf[sheet_y * sht->bxsize + sheet_x];
        if (color != sht->col_inv) {
          ctl->vram[vy * ctl->xsize + vx] = color;
        }
      }
    }
  }
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
  sht->vx0 = vx0;
  sht->vy0 = vy0;
  if (sht->height >= 0) sheet_refresh(ctl);
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht) {
  if (sht->height >= 0) sheet_updown(ctl, sht, -1);
  sht->flags = 0;
}
