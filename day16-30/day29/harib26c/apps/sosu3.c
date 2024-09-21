#include "../include/apilib.h"
#include "../include/mystdlib.h"

#define MAX 10000

void HariMain() {
  char s[8];
  api_initmalloc();
  char *flag = api_malloc(MAX);
  for (int i = 0; i < MAX; i++) flag[i] = 0;
  for (int i = 2; i < MAX; i++) {
    if (flag[i] == 0) {
      my_sprintf(s, "%d ", i);
      api_putstr0(s);
      for (int j = i * 2; j < MAX; j += i) flag[j] = 1;
    }
  }
  api_end();
}
