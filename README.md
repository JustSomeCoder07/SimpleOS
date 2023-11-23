# SimpleOS
[![Build SimpleOS](https://github.com/JustSomeCoder07/SimpleOS/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/JustSomeCoder07/SimpleOS/actions/workflows/build.yml)

SimpleOS is an operating system focused on simplicity, in terms of both usage and development.

## Building
### Compiling
SimpleOS is meant to be built under Ubuntu. With some adaptation it should be possible to build it under all common Unix systems, but this has not been tested.

First of all, you'll need to install the following packages:
```
apt install build-essential nasm
```
After that, you're able to build SimpleOS just by running `make`.

### Creating a Binary Image
First of all, you'll need to build the bootloader as described above. To create a binary image, you need to run `make floppy`. The resulting file *floppy.img* can then be flashed to a 1.44 MiB floppy disk or used in Qemu.

Alternatively, you can also create the image by yourself. You'll need to format a floppy disk using the filesystem FAT-12. After that, you'll need to write the contents of the file *BOOTSECT.SYS* to the bootsector of the floppy disk. Lastly, you'll need to copy the file *BOOTMGR.SYS* to the floppy disk. The creation of ISO images is only listed for completeness.

## Running
### On an emulator
It is recommended to use Qemu for testing SimpleOS. To run Qemu, you'll need to install the following packages:
```
apt install qemu qemu-system-i386
```
After that, you should be able to run SimpleOS by running ```make run```.

### On a real device
The only hard requirements to run SimpleOS are a x86 CPU and a computer with support for BIOS.
