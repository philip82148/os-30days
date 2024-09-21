#include <stdarg.h>
#include <stdbool.h>

int to_dec_asc(char *str, int len, char fill, int num) {
  bool is_negative = num < 0;
  if (is_negative) num = -num;

  int buf[20];
  int i = 0;
  while (1) {
    buf[i++] = num % 10 + '0';
    num /= 10;
    if (!num) break;
  }
  if (is_negative) buf[i++] = '-';

  int digits = i;
  if (len == 0) len = digits;
  else if (len < digits) len = digits;

  while (i--) str[len - 1 - i] = buf[i];

  i = len - digits;
  while (i--) str[i] = fill;

  return len;
}

int to_hex_asc(char *str, int len, char fill, bool is_upper, unsigned int num) {
  int buf[20];
  int i = 0;
  while (1) {
    int hex = num % 16;
    buf[i++] = hex < 10 ? hex + '0' : hex - 10 + (is_upper ? 'A' : 'a');
    num /= 16;
    if (!num) break;
  }

  int digits = i;
  if (len == 0) len = digits;
  else if (len < digits) len = digits;

  while (i--) str[len - 1 - i] = buf[i];

  i = len - digits;
  while (i--) str[i] = fill;

  return len;
}

void my_vsprintf(char *str, const char *fmt, va_list list) {
  va_list copy;
  va_copy(copy, list);

  while (*fmt) {
    if (*fmt == '%') {
      const char *fmt_start = fmt;
      fmt++;
      if (*fmt == '%') {
        *(str++) = *(fmt++);
        continue;
      }

      char fill = *fmt == '0' || *fmt == ' ' ? *fmt++ : ' ';
      int len = 0;
      while (*fmt >= '0' && *fmt <= '9') len = len * 10 + *fmt++ - '0';

      switch (*fmt++) {
        case 'd':
          str += to_dec_asc(str, len, fill, va_arg(copy, int));
          continue;
        case 'x':
          str += to_hex_asc(str, len, fill, false, va_arg(copy, int));
          continue;
        case 'X':
          str += to_hex_asc(str, len, fill, true, va_arg(copy, int));
          continue;
        default:
          fmt = fmt_start;
      }
    }

    *(str++) = *(fmt++);
  }
  *str = 0x00;

  va_end(copy);
}

void my_sprintf(char *str, const char *fmt, ...) {
  va_list list;
  va_start(list, fmt);
  my_vsprintf(str, fmt, list);
  va_end(list);
}
