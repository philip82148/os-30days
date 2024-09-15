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

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1) {
  if (vx0 < 0) vx0 = 0;
  if (vy0 < 0) vy0 = 0;
  if (vx1 > ctl->xsize) vx1 = ctl->xsize;
  if (vy1 > ctl->ysize) vy1 = ctl->ysize;

  for (int height = 0; height <= ctl->top; height++) {
    struct SHEET *sht = ctl->sheets[height];
    int sht_x0 = vx0 - sht->vx0;
    int sht_y0 = vy0 - sht->vy0;
    int sht_x1 = vx1 - sht->vx0;
    int sht_y1 = vy1 - sht->vy0;
    if (sht_x0 < 0) sht_x0 = 0;
    if (sht_y0 < 0) sht_y0 = 0;
    if (sht_x1 > sht->bxsize) sht_x1 = sht->bxsize;
    if (sht_y1 > sht->bysize) sht_y1 = sht->bysize;
    for (int sht_y = sht_y0; sht_y < sht_y1; sht_y++) {
      int vy = sht->vy0 + sht_y;
      for (int sht_x = sht_x0; sht_x < sht_x1; sht_x++) {
        int vx = sht->vx0 + sht_x;
        if (vx >= vx0 && vx < vx1 && vy >= vy0 && vy < vy1) {
          unsigned char color = sht->buf[sht_y * sht->bxsize + sht_x];
          if (color != sht->col_inv) ctl->vram[vy * ctl->xsize + vx] = color;
        }
      }
    }
  }
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
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
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
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
  }
}

void sheet_refresh(
    struct SHTCTL *ctl, struct SHEET *sht, int sht_x0, int sht_y0, int sht_x1, int sht_y1
) {
  if (sht->height >= 0)
    sheet_refreshsub(
        ctl, sht->vx0 + sht_x0, sht->vy0 + sht_y0, sht->vx0 + sht_x1, sht->vy0 + sht_y1
    );
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
  int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
  sht->vx0 = vx0;
  sht->vy0 = vy0;

  if (sht->height >= 0) {
    sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
    sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
  }
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht) {
  if (sht->height >= 0) sheet_updown(ctl, sht, -1);
  sht->flags = 0;
}
