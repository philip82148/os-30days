#include "../include/apilib.h"
#include "../include/mystdlib.h"

#define MAX 10000

void HariMain() {
  char flag[MAX], s[8];
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
