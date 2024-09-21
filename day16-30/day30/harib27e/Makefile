IMG=output/haribote-os.img
MYSTD_OBJECTS=output/lib/my_sprintf.o output/lib/my_strcmp.o output/lib/my_strncmp.o output/lib/my_memcmp.o \
			output/mystd/my_putchar.o output/mystd/my_exit.o output/mystd/my_printf.o output/mystd/my_malloc.o \
			output/mystd/my_free.o output/lib/my_strlen.o output/lib/my_strtol.o
API_OBJECTS=output/api/api001.obj output/api/api002.obj output/api/api003.obj output/api/api004.obj \
			output/api/api005.obj output/api/api006.obj output/api/api007.obj output/api/api008.obj \
			output/api/api009.obj output/api/api010.obj output/api/api011.obj output/api/api012.obj \
			output/api/api013.obj output/api/api014.obj output/api/api015.obj output/api/api016.obj \
			output/api/api017.obj output/api/api018.obj output/api/api019.obj output/api/api020.obj \
			output/api/api021.obj output/api/api022.obj output/api/api023.obj output/api/api024.obj \
			output/api/api025.obj output/api/api026.obj output/api/api027.obj
APPS=output/apps/a.tek output/apps/hello3.tek output/apps/hello4.tek output/apps/hello5.tek \
	 output/apps/winhelo.tek output/apps/winhelo2.tek output/apps/winhelo3.tek \
	 output/apps/star1.tek output/apps/stars.tek output/apps/stars2.tek \
	 output/apps/lines.tek output/apps/walk.tek \
	 output/apps/noodle.tek output/apps/beepdown.tek \
	 output/apps/color.tek output/apps/color2.tek \
	 output/apps/sosu.tek output/apps/sosu2.tek output/apps/sosu3.tek \
	 output/apps/typemkfl.tek output/apps/type.tek \
	 output/apps/iroha.tek output/apps/chklang.tek \
	 output/apps/notrec.tek output/apps/bball.tek output/apps/invader.tek \
	 output/apps/calc.tek output/apps/tview.tek output/apps/mmlplay.tek output/apps/gview.tek
SAMPLE_TXTS=ipl.nasm Makefile \
			../../sample_txts/shiftjis.txt ../../sample_txts/euc.txt ../../sample_txts/utf8.txt
MMLS=output/mml/daigo.tek output/mml/daiku.tek output/mml/fujisan.tek output/mml/kirakira.tek
IMAGES=output/images/cpp-dump.tek output/images/fukei.tek

all: $(IMG)
.PHONY: all

run: $(IMG)
	qemu-system-i386 -drive format=raw,if=floppy,file=$(IMG) -soundhw pcspk
.PHONY: run

rerun: re run
.PHONY: rerun

$(IMG): output/ipl.bin output/haribote.sys ../../fonts/nihongo.tek \
		$(APPS) $(SAMPLE_TXTS) $(MMLS) $(IMAGES)
	mformat -f 1440 -B $< -C -i $@
	mcopy $(filter-out $<, $^) -i $@ ::

output/%.tek: output/%.hrb
	mkdir -p $(@D)
	../../../z_tools_linux/bim2bin -osacmp in:$< out:$@

output/mml/%.tek: ../../mml-shiftjis/%.mml
	mkdir -p $(@D)
	../../../z_tools_linux/bim2bin -osacmp in:$< out:$@

output/images/%.tek: ../../images/%.jpg
	mkdir -p $(@D)
	../../../z_tools_linux/bim2bin -osacmp in:$< out:$@

output/images/%.tek: ../../images/%.bmp
	mkdir -p $(@D)
	../../../z_tools_linux/bim2bin -osacmp in:$< out:$@

output/apps/gview.hrb: output/libapi.a output/libmystd.a app.lds output/apps/gview/gview.o output/apps/gview/jpeg.o output/apps/gview/bmp.obj
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie -g -T $(filter-out %.a, $^) -Loutput -lapi -lmystd -o $@

output/%.hrb: app.lds output/%.obj
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie -g -T $^ -o $@

output/%.hrb: output/libapi.a output/libmystd.a app.lds output/%.o
	mkdir -p $(@D)
	gcc -nostdlib -m32 -fno-pie -g -T $(filter-out %.a, $^) -Loutput -lapi -lmystd -o $@

output/haribote.sys: output/asmhead.bin output/bootpack.hrb
	cat $^ > $@

output/bootpack.hrb: output/libmystd.a bootpack.lds output/c/bootpack.o output/nasmfunc.obj \
					 output/hankaku.o output/c/graphic.o output/c/dsctbl.o output/c/int.o \
					 output/c/fifo.o output/c/keyboard.o output/c/mouse.o output/c/memory.o \
					 output/c/sheet.o output/c/timer.o output/c/mtask.o output/c/window.o \
					 output/c/console.o output/c/file.o output/c/tek.o
	gcc -nostdlib -m32 -fno-pie -Wl,-Map=output/bootpack.map -g -T $(filter-out %.a, $^) \
		-Loutput -lmystd -o $@

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

output/libapi.a : $(API_OBJECTS)
	mkdir -p $(@D)
	ar rcs $@ $^

output/libmystd.a : $(MYSTD_OBJECTS)
	mkdir -p $(@D)
	ar rcs $@ $^

clean:
	rm -rf output
.PHONY: clean

re: clean all
.PHONY: re

.PHONY: exists
exists:
ifneq ("$(wildcard $(IMG))", "")
	@echo "true"
else
	@echo "false"
endif
