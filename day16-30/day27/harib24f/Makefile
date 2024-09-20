IMG=output/haribote-os.img
LIB_OBJECTS=output/lib/my_sprintf.o output/lib/my_strcmp.o output/lib/my_strncmp.o
API_OBJECTS=output/api/api001.obj output/api/api002.obj output/api/api003.obj output/api/api004.obj \
			output/api/api005.obj output/api/api006.obj output/api/api007.obj output/api/api008.obj \
			output/api/api009.obj output/api/api010.obj output/api/api011.obj output/api/api012.obj \
			output/api/api013.obj output/api/api014.obj output/api/api015.obj output/api/api016.obj \
			output/api/api017.obj output/api/api018.obj output/api/api019.obj output/api/api020.obj

all: $(IMG)
.PHONY: all

run: $(IMG)
	qemu-system-i386 -drive format=raw,if=floppy,file=$(IMG)
.PHONY: run

rerun: re run
.PHONY: rerun

$(IMG): output/ipl.bin output/haribote.sys ipl.nasm output/apps/hello.hrb output/apps/hello2.hrb \
        output/apps/hello3.hrb output/apps/a.hrb output/apps/hello4.hrb output/apps/hello5.hrb \
		output/apps/winhelo.hrb output/apps/winhelo2.hrb output/apps/winhelo3.hrb output/apps/star1.hrb \
		output/apps/stars.hrb output/apps/star2.hrb output/apps/lines.hrb output/apps/walk.hrb \
		output/apps/noodle.hrb output/apps/beepdown.hrb output/apps/color.hrb output/apps/color2.hrb \
		output/apps/crack7.hrb
	mformat -f 1440 -B $< -C -i $@
	mcopy $(filter-out $<, $^) Makefile -i $@ ::

output/haribote.sys: output/asmhead.bin output/bootpack.hrb
	cat $^ > $@

output/bootpack.hrb: bootpack.lds output/c/bootpack.o output/nasmfunc.obj output/hankaku.o \
					 $(LIB_OBJECTS) output/c/graphic.o output/c/dsctbl.o output/c/int.o \
					 output/c/fifo.o output/c/keyboard.o output/c/mouse.o output/c/memory.o \
					 output/c/sheet.o output/c/timer.o output/c/mtask.o output/c/window.o \
					 output/c/console.o output/c/file.o
	gcc -nostdlib -m32 -fno-pie -Wl,-Map=output/bootpack.map -g -T $^ -o $@

output/%.hrb: app.ld output/%.obj
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie -g -T $^ -o $@

output/%.hrb: %.nas
	mkdir -p $(@D)
	nasm $< -o $@ -l output/$*.lst

output/%.hrb : app.ld output/%.o $(LIB_OBJECTS) $(API_OBJECTS)
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie -g -T $^ -o $@

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

output/%.obj: %.nasm
	mkdir -p $(@D)
	nasm -f elf32 $< -o $@ -l output/$*.lst

output/%.bin: %.nasm
	mkdir -p $(@D)
	nasm $< -o $@ -l output/$*.lst

clean:
	rm -rf output
.PHONY: clean

re: clean all
.PHONY: re
