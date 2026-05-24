export PATH="$HOME/opt/cross/bin:$PATH"
mkdir -p build
cd build && rm -rf *
i686-elf-as ../boot.s -o boot.o
i686-elf-gcc -c ../kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T ../linker.ld -o myos -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

#Check if valid Multiboot header
if grub-file --is-x86-multiboot myos; then
  echo multiboot confirmed
else
  echo the file is not multiboot
fi

mkdir -p isodir/boot/grub
cp myos isodir/boot/myos
cp ../grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o myos.iso isodir

qemu-system-i386 -cdrom myos.iso
#qemu-system-i386 -kernel myos

#For usb
#sudo dd if=myos.iso of=/dev/sdx && sync
