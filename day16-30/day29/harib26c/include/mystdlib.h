#include <stdarg.h>

void my_sprintf(char *str, const char *fmt, ...);
void my_vsprintf(char *str, const char *fmt, va_list list);
int my_strcmp(const char *s1, const char *s2);
int my_strncmp(const char *s1, const char *s2, int n);

int my_putchar(int c);
void my_exit(int status);
void my_printf(const char *fmt, ...);
void *my_malloc(int size);
