#include <stdarg.h>

#include "../include/apilib.h"
#include "../include/mystdlib.h"

void my_printf(const char *fmt, ...) {
  char s[1000];

  va_list list;
  va_start(list, fmt);
  my_vsprintf(s, fmt, list);
  api_putstr0(s);
  va_end(list);
}
