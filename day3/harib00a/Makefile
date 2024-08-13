IMG=output/haribote-os.img

all: $(IMG)
.PHONY: all

run: $(IMG)
	qemu-system-i386 -drive format=raw,if=floppy,file=$(IMG)
.PHONY: run

$(IMG): output/ipl.bin
	dd if=output/ipl.bin of=$@
	dd of=$@ bs=1 count=0 seek=1474560

output/ipl.bin: ipl.nasm
	mkdir -p output
	nasm ipl.nasm -o $@ -l output/ipl.lst

clean:
	rm -rf output
.PHONY: clean

re: clean all
.PHONY: re
