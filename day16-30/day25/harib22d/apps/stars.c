int api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc();
char *api_malloc(int size);
void api_point(int win, int x, int y, int col);
void api_end();

int rand();  // 0~32767

void HariMain() {
  char *buf;
  int win, i, x, y;
  api_initmalloc();
  buf = api_malloc(150 * 100);
  win = api_openwin(buf, 150, 100, -1, "stars");
  api_boxfilwin(win, 6, 26, 143, 93, 0);
  for (i = 0; i < 50; i++) {
    x = (rand() % 137) + 6;
    y = (rand() % 67) + 26;
    api_point(win, x, y, 3);
  }
  api_end();
}

/* https://stackoverflow.com/questions/4768180/rand-implementation/4768194 */
int rand() {  // RAND_MAX assumed to be 32767
  static unsigned long int next = 1;
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}
