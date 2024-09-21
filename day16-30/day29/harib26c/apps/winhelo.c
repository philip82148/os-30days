#include "../include/apilib.h"

void HariMain() {
  char buf[150 * 50];
  int win = api_openwin(buf, 150, 50, -1, "hello");
  for (;;) {
    if (api_getkey(1) == 0x0a) break;
  }
  api_end();
}
