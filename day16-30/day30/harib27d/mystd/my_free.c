#include "../include/apilib.h"
#include "../include/mystdlib.h"

void my_free(void *p) {
  char *q = p;
  if (q != 0) {
    q -= 16;
    int size = *((int *)q);
    api_free(q, size + 16);
  }
}
