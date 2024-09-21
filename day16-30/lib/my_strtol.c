#define ERANGE    34
#define ULONG_MAX (+0xffffffff)
#define LONG_MAX  (+0x7fffffff)
#define LONG_MIN  (-0x7fffffff)

int errno = 0;

static int prefix(int c) {
  signed char base = 0;
  if ('a' <= c && c <= 'z') c += 'A' - 'a';
  if (c == 'B') base = 2;
  if (c == 'D') base = 10;
  if (c == 'O') base = 8;
  if (c == 'X') base = 16;
  return base;
}

unsigned long my_strtoul0(const unsigned char **ps, int base, unsigned char *errflag) {
  const unsigned char *s = *ps;
  unsigned long val = 0, max;
  int digit;
  if (base == 0) {
    base += 10;
    if (*s == '0') {
      base = prefix(*(s + 1));
      if (base == 0) base += 8; /* base = 8; */
    }
  }
  if (*s == '0') {
    if (base == prefix(*(s + 1))) s += 2;
  }
  max = ULONG_MAX / base;
  *errflag = 0;
  for (;;) {
    digit = 99;
    if ('0' <= *s && *s <= '9') digit = *s - '0';
    if ('A' <= *s && *s <= 'Z') digit = *s - ('A' - 10);
    if ('a' <= *s && *s <= 'z') digit = *s - ('a' - 10);
    if (digit >= base) break;
    if (val > max) goto err;
    val *= base;
    if (ULONG_MAX - val < (unsigned long)digit) {
    err:
      *errflag = 1;
      val = ULONG_MAX;
    } else val += digit;
    s++;
  }
  *ps = s;
  return val;
}

long my_strtol(const char *s, const char **endp, int base) {
  const char *s0 = s, *s1;
  char sign = 0, errflag;
  unsigned long val;
  while (*s != '\0' && *s <= ' ') s++;
  if (*s == '-') {
    sign = 1;
    s++;
  }
  while (*s != '\0' && *s <= ' ') s++;
  s1 = s;
  val = my_strtoul0((const unsigned char **)&s, base, (unsigned char *)&errflag);
  if (s == s1) s = s0;
  if (endp) *endp = s;
  if (val > LONG_MAX) {
    errflag = 1;
    val = LONG_MAX;
    if (sign) val = LONG_MIN;
  }
  if (errflag == 0 && sign != 0) val = -val;
  if (errflag) errno = ERANGE;
  return val;
}
