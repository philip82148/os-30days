#include <stdio.h>
#include <string.h>

int main() {
  FILE *in_txt;
  if ((in_txt = fopen("hankaku/hankaku.txt", "r")) == NULL) {
    printf("Can't open the input file.\n");
    return 0;
  }

  FILE *out_c;
  if ((out_c = fopen("output/hankaku.c", "w")) == NULL) {
    printf("Can't open the output file.\n");
    return 0;
  }

  fputs("char hankaku[4096] =  {\n  ", out_c);

  char line[256];
  fgets(line, sizeof(line), in_txt);
  for (int ascii = 0; ascii < 256; ascii++) {
    fgets(line, sizeof(line), in_txt);
    fgets(line, sizeof(line), in_txt);

    for (int row = 0; row < 16; row++) {
      fgets(line, sizeof(line), in_txt);

      char binary[9];
      for (int col = 0; col < 8; col++) binary[col] = line[col] == '*' ? '1' : '0';
      binary[8] = '\0';

      if (row > 0) fputs(", ", out_c);
      fputs("0b", out_c);
      fputs(binary, out_c);
    }
    if (ascii < 255) fputs(",\n  ", out_c);
  }

  fputs("\n};\n", out_c);

  fclose(in_txt);
  fclose(out_c);
}
