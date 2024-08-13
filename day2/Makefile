HELLOOS4=output/helloos4.img

helloos4: $(HELLOOS4)
	qemu-system-i386 -drive format=raw,if=floppy,file=$(HELLOOS4)
.PHONY: helloos4

$(HELLOOS4): output/ipl.bin
	dd if=output/ipl.bin of=$@
	dd of=$@ bs=1 count=0 seek=1474560

output/ipl.bin: ipl.nasm
	mkdir -p output
	nasm ipl.nasm -o $@ -l output/ipl.lst

clean:
	rm -rf output
.PHONY: clean
