all: boot1 boot2

boot1: kernel.exe boot1.asm
	nasm -l boot1.list -DENTRY=0x$(shell nm kernel.exe | grep main | cut -d " " -f 1) boot1.asm

boot2: kernel.exe
	objcopy -j .text* -j .data* -j .rodata* -S -O binary kernel.exe boot2

kernel.exe: kernel_c.o kernel_asm.o
	ld  -g -melf_i386 -Ttext 0x10000 -e main -o kernel.exe kernel_asm.o kernel_c.o

kernel_c.o: kernel.c
	gcc -g -m32 -c -o kernel_c.o kernel.c

kernel_asm.o: kernel.asm
	nasm -g -f elf -F dwarf -o kernel_asm.o kernel.asm

clean:
	rm -f kernel_asm.o kernel_c.o a.img kernel.exe 

install:
	bximage -mode=create -fd=1.44M -q a.img 
	/sbin/mkdosfs a.img
	dd if=boot1 of=a.img bs=1 count=512 conv=notrunc
	mcopy -o boot2 a:BOOT2

run:
	qemu-system-i386 -boot a -fda a.img

debug:
	qemu-system-i386 -S -s -boot a -fda a.img
