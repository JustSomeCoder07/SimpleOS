VOLUME_LABEL?="SIMPLEOS"
SERIAL_NUMBER?="$(shell uuidgen | head -c8)"

.PHONY: bootloader floppy run clean

all: run

bootloader:
	nasm -f bin boot/stage1/bootsector.asm -i boot/stage1 -o BOOTSECT.SYS \
	-D SERIAL_NUMBER=0x${SERIAL_NUMBER} -D VOLUME_LABEL'${VOLUME_LABEL}'
	nasm -f bin boot/stage2/boot.asm -i boot/stage2 -o BOOTMGR.SYS \
	-D SERIAL_NUMBER=0x${SERIAL_NUMBER} -D VOLUME_LABEL'${VOLUME_LABEL}'

floppy: bootloader
	dd if=/dev/zero of=floppy.img bs=512 count=2880
	mkfs.vfat -F 12 floppy.img -f 2 -h 0 -i $(SERIAL_NUMBER) -I -n $(VOLUME_LABEL) -r 224 -R 1 -s 1 -S 512
	dd if=BOOTSECT.SYS of=floppy.img conv=notrunc
	sudo mkdir -p /mnt/simpleos/floppy1
	sudo mount -o loop floppy.img /mnt/simpleos/floppy1
	sudo cp BOOTMGR.SYS /mnt/simpleos/floppy1
	sudo umount /mnt/simpleos/floppy1
	sudo rm -rf /mnt/simpleos

run: floppy
	qemu-system-i386 -drive file=floppy.img,format=raw,if=floppy

clean:
	rm -f MBR.SYS floppy.img
