IMG=output/haribote-os.img

all: $(IMG)
.PHONY: all

run: $(IMG)
	qemu-system-i386 -drive format=raw,if=floppy,file=$(IMG)
.PHONY: run

rerun: re run
.PHONY: rerun

$(IMG): output/ipl.bin output/haribote.sys ipl.nasm output/hlt.hrb
	mformat -f 1440 -B output/ipl.bin -C -i $@
	mcopy output/haribote.sys ipl.nasm output/hlt.hrb Makefile -i $@ ::

output/haribote.sys: output/asmhead.bin output/bootpack.hrb
	cat $^ > $@

output/bootpack.hrb: bootpack.lds output/c/bootpack.o output/nasmfunc.o output/hankaku.o \
					 output/lib/my_sprintf.o output/c/graphic.o output/c/dsctbl.o output/c/int.o \
					 output/c/fifo.o output/c/keyboard.o output/c/mouse.o output/c/memory.o \
					 output/c/sheet.o output/c/timer.o output/c/mtask.o output/lib/my_strcmp.o \
					 output/lib/my_strncmp.o output/c/window.o output/c/console.o output/c/file.o
	gcc -nostdlib -m32 -fno-pie -T $^ -o $@

output/hlt.hrb: hlt.nasm
	nasm $< -o $@ -l output/hlt.lst

output/%.o: %.c
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie $< -c -o $@

output/lib/%.o: ../../lib/%.c
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie $< -c -o $@

output/hankaku.o: output/hankaku.c
	gcc -nostdlib -m32 -fno-pie $< -c -o $@

output/hankaku.c: output/make_hankaku_c hankaku/hankaku.txt
	./output/make_hankaku_c

output/make_hankaku_c: hankaku/make_hankaku_c.c
	mkdir -p $(@D)
	gcc $< -o $@

output/nasmfunc.o: nasmfunc.nasm
	mkdir -p $(@D)
	nasm -f elf32 $< -o $@ -l output/nasmfunc.lst

output/%.bin: %.nasm
	mkdir -p $(@D)
	nasm $< -o $@ -l output/$*.lst

clean:
	rm -rf output
.PHONY: clean

re: clean all
.PHONY: re
