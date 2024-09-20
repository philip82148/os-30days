#include "apilib.h"
char buf[150 * 50];

void HariMain() {
  int win = api_openwin(buf, 150, 50, -1, "hello");
  for (;;) {
    if (api_getkey(1) == 0x0a) break;
  }
  api_end();
}
