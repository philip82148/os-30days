#include "../include/apilib.h"

void HariMain() {
  static char s[9] = {
      0xb2, 0xdb, 0xca, 0xc6, 0xce, 0xcd, 0xc4, 0x0a, 0x00
  };  // Hankaku "i ro ha ni ho he to" + new line + 0
  api_putstr0(s);
  api_end();
}
