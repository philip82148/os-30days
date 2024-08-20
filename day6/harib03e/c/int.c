#include "bootpack.h"

// Initialization of PIC
void init_pic() {
  io_out8(PIC0_IMR, 0xff);  // 全割り込み禁止
  io_out8(PIC1_IMR, 0xff);  // 全割り込み禁止

  io_out8(PIC0_ICW1, 0x11);    // エッジトリガモード
  io_out8(PIC0_ICW2, 0x20);    // IRQ 0-7 -> INT 20-27
  io_out8(PIC0_ICW3, 1 << 2);  // connect PIC1 -> IRQ2
  io_out8(PIC0_ICW4, 0x01);    // ノンバッファモード

  io_out8(PIC1_ICW1, 0x11);  // エッジトリガモード
  io_out8(PIC1_ICW2, 0x28);  // IRQ 8-15 -> INT 28-2f
  io_out8(PIC1_ICW3, 2);     // connect PIC1 -> IRQ2
  io_out8(PIC0_ICW4, 0x01);  // ノンバッファモード

  io_out8(PIC0_IMR, 0xfb);  // PIC1以外割り込み禁止
  io_out8(PIC1_IMR, 0xff);  // 全割り込み禁止
}

// PS/2キーボード割込み
void inthandler21(int *esp) {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  // boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
  // putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 21 (IRQ-1) : PS/2 keyboard");
  for (int i = 0; i < 16; ++i)
    putfonts8_asc(binfo->vram, binfo->scrnx, i << 3, i << 3, i, "INT 21 (IRQ-1) : PS/2 keyboard");

  for (;;) io_hlt();
}

// PS/2マウス割込み
void inthandler2c(int *esp) {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_C6C6C6, "INT 2C (IRQ-12) : PS/2 mouse");
  for (;;) io_hlt();
}

// Prevention for unsuccessful interrupt
void inthandler27(int *esp) { io_out8(PIC0_OCW2, 0x67); }
