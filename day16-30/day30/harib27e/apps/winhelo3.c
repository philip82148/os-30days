#include "../include/apilib.h"

void HariMain() {
  api_initmalloc();
  char *buf = api_malloc(150 * 50);
  int win = api_openwin(buf, 150, 50, -1, "hello");
  api_boxfilwin(win, 8, 36, 141, 43, 6);
  api_putstrwin(win, 28, 28, 0, 12, "hello, world");
  for (;;) {
    if (api_getkey(1) == 0x0a) break;
  }
  api_end();
}
