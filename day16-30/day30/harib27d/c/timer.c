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
  for (int i = 0; i < MAX_TIMER; i++) timerctl.timers0[i].flags = 0;
  struct TIMER *t = timer_alloc();
  t->timeout = 0xffffffff;
  t->flags = TIMER_FLAGS_USING;
  t->next = 0;
  timerctl.t0 = t;
  timerctl.next = 0xffffffff;  // No timer at first
}

struct TIMER *timer_alloc() {
  for (int i = 0; i < MAX_TIMER; i++) {
    if (timerctl.timers0[i].flags == 0) {
      timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
      timerctl.timers0[i].flags2 = 0;
      return &timerctl.timers0[i];
    }
  }
  return 0;  // Cannot find
}

void timer_free(struct TIMER *timer) {
  timer->flags = 0;  // Not in use
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
  timer->fifo = fifo;
  timer->data = data;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
  timer->timeout = timeout + timerctl.count;
  timer->flags = TIMER_FLAGS_USING;
  int e = io_load_eflags();
  io_cli();

  struct TIMER *t = timerctl.t0;
  // Put into first
  if (timer->timeout <= t->timeout) {
    timerctl.t0 = timer;
    timer->next = t;
    timerctl.next = timer->timeout;
    io_store_eflags(e);
    return;
  }

  // Search where to insert
  struct TIMER *s;
  for (;;) {
    s = t;
    t = t->next;
    if (t == 0) break;  // End up last
    // insert between s and t
    if (timer->timeout <= t->timeout) {
      s->next = timer;
      timer->next = t;
      io_store_eflags(e);
      return;
    }
  }
}

void inthandler20(int *esp) {
  io_out8(PIC0_OCW2, 0x60);  // Notify to PIC that IRQ-00 received
  timerctl.count++;
  if (timerctl.next > timerctl.count) return;

  struct TIMER *timer = timerctl.t0;  // Address of first timer
  char ts = 0;
  for (;;) {
    if (timerctl.count < timer->timeout) break;

    // Timeout
    timer->flags = TIMER_FLAGS_ALLOC;
    if (timer == task_timer) {
      ts = 1;  // mt_timerがタイムアウトした
    } else {
      fifo32_put(timer->fifo, timer->data);
    }
    timer = timer->next;  // 次のタイマの番地をtimerに代入
  }
  timerctl.t0 = timer;
  timerctl.next = timerctl.t0->timeout;

  if (ts) task_switch();
}

int timer_cancel(struct TIMER *timer) {
  int e = io_load_eflags();
  io_cli();
  if (timer->flags == TIMER_FLAGS_USING) {  // Need to cancel?
    if (timer == timerctl.t0) {             // The first timer
      struct TIMER *t = timer->next;
      timerctl.t0 = t;
      timerctl.next = t->timeout;
    } else {  // Other than first
      struct TIMER *t = timerctl.t0;
      for (;;) {
        if (t->next == timer) {
          break;
        }
        t = t->next;
      }
      t->next = timer->next;
    }
    timer->flags = TIMER_FLAGS_ALLOC;
    io_store_eflags(e);
    return 1;
  }
  io_store_eflags(e);
  return 0;
}

void timer_cancelall(struct FIFO32 *fifo) {
  int e = io_load_eflags();
  io_cli();
  for (int i = 0; i < MAX_TIMER; i++) {
    struct TIMER *t = &timerctl.timers0[i];
    if (t->flags != 0 && t->flags2 != 0 && t->fifo == fifo) {
      timer_cancel(t);
      timer_free(t);
    }
  }
  io_store_eflags(e);
}
