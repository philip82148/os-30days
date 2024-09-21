#include "apilib.h"

int my_sprintf(char *str, const char *fmt, ...);

void HariMain() {
  char s[12];
  int sec = 0, min = 0, hou = 0;

  api_initmalloc();
  char *buf = api_malloc(150 * 50);
  int win = api_openwin(buf, 150, 50, -1, "noodle");
  int timer = api_alloctimer();
  api_inittimer(timer, 128);
  for (;;) {
    my_sprintf(s, "%5d:%02d:%02d", hou, min, sec);
    api_boxfilwin(win, 28, 27, 115, 41, 7);
    api_putstrwin(win, 28, 27, 0, 11, s);
    api_settimer(timer, 100);
    if (api_getkey(1) != 128) break;
    sec++;
    if (sec == 60) {
      sec = 0;
      min++;
      if (min == 60) {
        min = 0;
        hou++;
      }
    }
  }
  api_end();
}
