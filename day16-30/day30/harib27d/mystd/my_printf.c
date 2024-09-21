#include <stdarg.h>

#include "../include/apilib.h"
#include "../include/mystdlib.h"

int my_printf(const char *fmt, ...) {
  char s[1000];

  va_list list;
  va_start(list, fmt);
  int len = my_vsprintf(s, fmt, list);
  api_putstr0(s);
  va_end(list);

  return len;
}
