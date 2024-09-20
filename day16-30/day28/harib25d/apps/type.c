#include "apilib.h"

void HariMain() {
  char cmdline[30], *p;
  api_cmdline(cmdline, 30);
  for (p = cmdline; *p > ' '; p++);  // Skip until space
  for (; *p == ' '; p++);            // Skip space
  int fh = api_fopen(p);
  if (fh != 0) {
    for (;;) {
      char c;
      if (api_fread(&c, 1, fh) == 0) break;
      api_putchar(c);
    }
  } else {
    api_putstr0("File not found.\n");
  }
  api_end();
}
