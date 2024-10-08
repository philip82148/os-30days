#include "../include/apilib.h"
#include "../include/mystdlib.h"

void waittimer(int timer, int time);
void end(char *s);

void HariMain(void) {
  char winbuf[256 * 112], txtbuf[100 * 1024];
  char s[32], *p, *r;
  int win, timer, i, j, t = 120, l = 192 / 4, o = 4, q = 7, note_old = 0;

  // Note Numer <=> Frequency(mHz)
  static int tonetable[12] = {
      1071618315,
      1135340056,
      1202850889,
      1274376125,
      1350154473,
      1430438836,
      1515497155,
      1605613306,
      1701088041,
      1802240000,
      1909406767,
      2022946002
  };
  static int notetable[7] = {+9, +11, +0 /* C */, +2, +4, +5, +7};

  // Parse commandline
  api_cmdline(s, 30);
  for (p = s; *p > ' '; p++) {
  }  // Skip until space
  for (; *p == ' '; p++) {
  }  // Skip until space
  i = my_strlen(p);
  if (i > 12) {
  file_error:
    end("file open error.\n");
  }
  if (i == 0) {
    end(0);
  }

  // Prepare window
  win = api_openwin(winbuf, 256, 112, -1, "mmlplay");
  api_putstrwin(win, 128, 32, 0, i, p);
  api_boxfilwin(win, 8, 60, 247, 76, 7);
  api_boxfilwin(win, 6, 86, 249, 105, 7);

  // Read file
  i = api_fopen(p);
  if (i == 0) {
    goto file_error;
  }
  j = api_fsize(i, 0);
  if (j >= 100 * 1024) {
    j = 100 * 1024 - 1;
  }
  api_fread(txtbuf, j, i);
  api_fclose(i);
  txtbuf[j] = 0;
  r = txtbuf;
  i = 0;                            // Normal mode
  for (p = txtbuf; *p != 0; p++) {  // Remove comment, space
    if (i == 0 && *p > ' ') {       // Not space nor new line
      if (*p == '/') {
        if (p[1] == '*') {
          i = 1;
        } else if (p[1] == '/') {
          i = 2;
        } else {
          *r = *p;
          if ('a' <= *p && *p <= 'z') {
            *r += 'A' - 'a';  // Lowercase to uppercase
          }
          r++;
        }
      } else if (*p == 0x22) {
        *r = *p;
        r++;
        i = 3;
      } else {
        *r = *p;
        r++;
      }
    } else if (i == 1 && *p == '*' && p[1] == '/') {  // Block comment
      p++;
      i = 0;
    } else if (i == 2 && *p == 0x0a) {  // Line comment
      i = 0;
    } else if (i == 3) {  // Strings
      *r = *p;
      r++;
      if (*p == 0x22) {
        i = 0;
      } else if (*p == '%') {
        p++;
        *r = *p;
        r++;
      }
    }
  }
  *r = 0;

  // Timer
  timer = api_alloctimer();
  api_inittimer(timer, 128);

  // Main
  p = txtbuf;
  for (;;) {
    if (('A' <= *p && *p <= 'G') || *p == 'R') {  // Note
      // Calculate frequency
      if (*p == 'R') {
        i = 0;
        s[0] = 0;
      } else {
        i = o * 12 + notetable[*p - 'A'] + 12;
        s[0] = 'O';
        s[1] = '0' + o;
        s[2] = *p;
        s[3] = ' ';
        s[4] = 0;
      }
      p++;
      if (*p == '+' || *p == '-' || *p == '#') {
        s[3] = *p;
        if (*p == '-') {
          i--;
        } else {
          i++;
        }
        p++;
      }
      if (i != note_old) {
        api_boxfilwin(win + 1, 32, 36, 63, 51, 8);
        if (s[0] != 0) {
          api_putstrwin(win + 1, 32, 36, 10, 4, s);
        }
        api_refreshwin(win, 32, 36, 64, 52);
        if (28 <= note_old && note_old <= 107) {
          api_boxfilwin(win, (note_old - 28) * 3 + 8, 60, (note_old - 28) * 3 + 10, 76, 7);
        }
        if (28 <= i && i <= 107) {
          api_boxfilwin(win, (i - 28) * 3 + 8, 60, (i - 28) * 3 + 10, 76, 4);
        }
        if (s[0] != 0) {
          api_beep(tonetable[i % 12] >> (17 - i / 12));
        } else {
          api_beep(0);
        }
        note_old = i;
      }
      // Calculate wave length
      if ('0' <= *p && *p <= '9') {
        i = 192 / my_strtol(p, (const char **)&p, 10);
      } else {
        i = l;
      }
      for (; *p == '.';) {
        p++;
        i += i / 2;
      }
      i *= (60 * 100 / 48);
      i /= t;
      if (s[0] != 0 && q < 8 && *p != '&') {
        j = i * q / 8;
        waittimer(timer, j);
        api_boxfilwin(win, 32, 36, 63, 51, 8);
        if (28 <= note_old && note_old <= 107) {
          api_boxfilwin(win, (note_old - 28) * 3 + 8, 60, (note_old - 28) * 3 + 10, 76, 7);
        }
        note_old = 0;
        api_beep(0);
      } else {
        j = 0;
        if (*p == '&') {
          p++;
        }
      }
      waittimer(timer, i - j);
    } else if (*p == '<') {  // octave--
      p++;
      o--;
    } else if (*p == '>') {  // octave++
      p++;
      o++;
    } else if (*p == 'O') {  // Set octave
      o = my_strtol(p + 1, (const char **)&p, 10);
    } else if (*p == 'Q') {  // Set Q param
      q = my_strtol(p + 1, (const char **)&p, 10);
    } else if (*p == 'L') {  // Default sound length
      l = my_strtol(p + 1, (const char **)&p, 10);
      if (l == 0) {
        goto syntax_error;
      }
      l = 192 / l;
      for (; *p == '.';) {
        p++;
        l += l / 2;
      }
    } else if (*p == 'T') {  // Tempo
      t = my_strtol(p + 1, (const char **)&p, 10);
    } else if (*p == '$') {  // Extended command
      if (p[1] == 'K') {     // Karaoke
        p += 2;
        for (; *p != 0x22; p++) {
          if (*p == 0) {
            goto syntax_error;
          }
        }
        p++;
        for (i = 0; i < 32; i++) {
          if (*p == 0) {
            goto syntax_error;
          }
          if (*p == 0x22) {
            break;
          }
          if (*p == '%') {
            s[i] = p[1];
            p += 2;
          } else {
            s[i] = *p;
            p++;
          }
        }
        if (i > 30) {
          end("karaoke too long.\n");
        }
        api_boxfilwin(win + 1, 8, 88, 247, 103, 7);
        s[i] = 0;
        if (i != 0) {
          api_putstrwin(win + 1, 128 - i * 4, 88, 0, i, s);
        }
        api_refreshwin(win, 8, 88, 248, 104);
      }
      for (; *p != ';'; p++) {
        if (*p == 0) {
          goto syntax_error;
        }
      }
      p++;
    } else if (*p == 0) {
      p = txtbuf;
    } else {
    syntax_error:
      end("mml syntax error.\n");
    }
  }
}

void waittimer(int timer, int time) {
  int i;
  api_settimer(timer, time);
  for (;;) {
    i = api_getkey(1);
    if (i == 'Q' || i == 'q') {
      api_beep(0);
      api_end();
    }
    if (i == 128) {
      return;
    }
  }
}

void end(char *s) {
  if (s != 0) {
    api_putstr0(s);
  }
  api_beep(0);
  api_end();
}
