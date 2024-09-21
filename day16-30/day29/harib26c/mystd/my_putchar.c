#include "../include/apilib.h"
#include "../include/mystdlib.h"

int my_putchar(int c) {
  api_putchar(c);
  return c;
}
