/*
 * bootpack.h
 */

// lib/my_sprint.c --------------------------------------------------------------------------------
void my_sprintf(char *str, const char *fmt, ...);

// asmhead.nas ------------------------------------------------------------------------------------
#define ADR_BOOTINFO 0x00000ff0
struct BOOTINFO {
  unsigned char cyls, leds, vmode, reserve;
  short scrnx, scrny;
  unsigned char *vram;
};

// nasmfunc.nas -----------------------------------------------------------------------------------
void io_hlt();
void io_cli();
void io_sti();
void io_stihlt();
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0();
void store_cr0(int cr0);
void asm_inthandler21();
void asm_inthandler27();
void asm_inthandler2c();

// fifo.c -----------------------------------------------------------------------------------------
struct FIFO8 {
  unsigned char *buf;
  int p, q, size, free, flags;
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);

// graphic.c --------------------------------------------------------------------------------------
#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

void init_palette();
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(unsigned char *vram, int x, int y);
void putfont8(unsigned char *vram, int xsize, int x, int y, unsigned char c, unsigned char *font);
void putfonts8_asc(unsigned char *vram, int xsize, int x, int y, unsigned char c, const char *s);
void init_mouse_cursor8(unsigned char *mouse, unsigned char bc);
void putblock8_8(
    unsigned char *vram,
    int vxsize,
    int pxsize,
    int pysize,
    int px0,
    int py0,
    unsigned char *buf,
    int bxsize
);

// dsctbl.c ---------------------------------------------------------------------------------------
#define ADR_IDT      0x0026f800  // 32-bit (4-byte)
#define LIMIT_IDT    0x000007ff
#define ADR_GDT      0x00270000
#define LIMIT_GDT    0x0000ffff
#define ADR_BOTPAK   0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_INTGATE32 0x008e

struct SEGMENT_DESCRIPTOR {
  short limit_low, base_low;
  unsigned char base_mid, access_right;
  unsigned char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
  short offset_low, selector;
  unsigned char dw_count, access_right;
  short offset_high;
};

void init_gdtidt();
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

// int.c ------------------------------------------------------------------------------------------
#define PIC0_ICW1 0x0020  // 16-bit (2-byte)
#define PIC0_OCW2 0x0020
#define PIC0_IMR  0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR  0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1
void init_pic();
void inthandler27(int *esp);

// keyboard.c -------------------------------------------------------------------------------------
#define PORT_KEYDAT 0x0060
#define PORT_KEYCMD 0x0064
extern struct FIFO8 keyfifo;
void inthandler21(int *esp);
void wait_KBC_sendready();
void init_keyboard();

// mouse.c ----------------------------------------------------------------------------------------
struct MOUSE_DEC {
  unsigned char buf[3], phase;
  int x, y, btn;
};
extern struct FIFO8 mousefifo;
void inthandler2c(int *esp);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);
