/*
 * timer.c
 */

#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

struct TIMERCTL timerctl;

void init_pit() {
  io_out8(PIT_CTRL, 0x34);
  io_out8(PIT_CNT0, 0x9c);
  io_out8(PIT_CNT0, 0x2e);
  timerctl.count = 0;
}

void inthandler20(int *esp) {
  io_out8(PIC0_OCW2, 0x60);  // Notify to PIC that IRQ-00 received
  timerctl.count++;
}
