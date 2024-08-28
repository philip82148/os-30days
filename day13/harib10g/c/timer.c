/*
 * timer.c
 */

#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

struct TIMERCTL timerctl;

void init_pit() {
  io_out8(PIT_CTRL, 0x34);
  io_out8(PIT_CNT0, 0x9c);
  io_out8(PIT_CNT0, 0x2e);
  timerctl.count = 0;
  timerctl.next = 0xffffffff;  // No timer at first
  timerctl.using_ = 0;
  for (int i = 0; i < MAX_TIMER; i++) {
    timerctl.timers0[i].flags = 0;  // Not in use
  }
}

struct TIMER *timer_alloc() {
  for (int i = 0; i < MAX_TIMER; i++) {
    if (timerctl.timers0[i].flags == 0) {
      timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
      return &timerctl.timers0[i];
    }
  }
  return 0;  // Cannot find
}

void timer_free(struct TIMER *timer) {
  timer->flags = 0;  // Not in use
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data) {
  timer->fifo = fifo;
  timer->data = data;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
  timer->timeout = timeout + timerctl.count;
  timer->flags = TIMER_FLAGS_USING;
  int e = io_load_eflags();
  io_cli();

  // Search where to insert
  int i;
  for (i = 0; i < timerctl.using_; i++) {
    if (timerctl.timers[i]->timeout >= timer->timeout) {
      break;
    }
  }

  // Adjust latters
  for (int j = timerctl.using_; j > i; j--) {
    timerctl.timers[j] = timerctl.timers[j - 1];
  }
  timerctl.using_++;
  // Insert between

  timerctl.timers[i] = timer;
  timerctl.next = timerctl.timers[0]->timeout;
  io_store_eflags(e);
}

void inthandler20(int *esp) {
  io_out8(PIC0_OCW2, 0x60);  // Notify to PIC that IRQ-00 received
  timerctl.count++;
  if (timerctl.next > timerctl.count) return;

  int i = 0;
  for (i = 0; i < timerctl.using_; i++) {
    // Every timer is active
    if (timerctl.timers[i]->timeout > timerctl.count) break;

    // Timeout
    timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
    fifo32_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
  }

  // i timers went timeout, adjust remains
  timerctl.using_ -= i;
  for (int j = 0; j < timerctl.using_; j++) timerctl.timers[j] = timerctl.timers[i + j];
  if (timerctl.using_ > 0) {
    timerctl.next = timerctl.timers[0]->timeout;
  } else {
    timerctl.next = 0xffffffff;
  }
}
