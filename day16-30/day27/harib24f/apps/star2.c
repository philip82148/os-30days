#include "apilib.h"

int rand();  // 0~32767

void HariMain() {
  api_initmalloc();
  char *buf = api_malloc(150 * 100);
  int win = api_openwin(buf, 150, 100, -1, "stars2");
  api_boxfilwin(win + 1, 6, 26, 143, 93, 0);
  for (int i = 0; i < 50; i++) {
    int x = (rand() % 137) + 6;
    int y = (rand() % 67) + 26;
    api_point(win + 1, x, y, 3);
  }
  api_refreshwin(win, 6, 26, 144, 94);
  api_end();
}

/* https://stackoverflow.com/questions/4768180/rand-implementation/4768194 */
int rand() {  // RAND_MAX assumed to be 32767
  static unsigned long int next = 1;
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}
