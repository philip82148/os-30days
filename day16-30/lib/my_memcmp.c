int my_memcmp(const void *d, const void *s, unsigned long sz) {
  const char *dp = (const char *)d;
  const char *sp = (const char *)s;
  while (sz--) {
    if (*dp != *sp) return *dp - *sp;
    dp++;
    sp++;
  }
  return 0;
}
