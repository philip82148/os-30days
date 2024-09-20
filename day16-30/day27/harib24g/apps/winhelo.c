#include "apilib.h"
char buf[150 * 50];

void HariMain() {
  int win = api_openwin(buf, 150, 50, -1, "hello");
  api_end();
}
