IMG=output/haribote-os.img

all: $(IMG)
.PHONY: all

run: $(IMG)
	qemu-system-i386 -drive format=raw,if=floppy,file=$(IMG)
.PHONY: run

$(IMG): output/ipl.bin output/haribote.sys
	mformat -f 1440 -B output/ipl.bin -C -i $@
	mcopy output/haribote.sys -i $@ ::

output/haribote.sys: output/asmhead.bin output/bootpack.bin
	cat output/asmhead.bin output/bootpack.bin > $@

output/bootpack.bin: bootpack.c bootpack.lds
	mkdir -p output
	gcc -nostdlib -m32 -fno-pie -T bootpack.lds bootpack.c -o $@

output/asmhead.bin: asmhead.nasm
	mkdir -p output
	nasm asmhead.nasm -o $@ -l output/asmhead.lst

output/ipl.bin: ipl.nasm
	mkdir -p output
	nasm ipl.nasm -o $@ -l output/ipl.lst

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
