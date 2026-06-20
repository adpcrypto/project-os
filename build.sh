export PATH="$HOME/opt/cross/bin:$PATH"
mkdir -p build
cd build && rm -rf *


#x86 build
# nasm -f elf64 ../boot.s -o boot.o
# x86_64-elf-gcc -c ../kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
# x86_64-elf-gcc -T ../linker.ld -o myos -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

python3 ../tools/gen_script.py symbols.c
cmake .. -DCMAKE_TOOLCHAIN_FILE=../x86_64-toolchain.cmake
make  #VERBOSE=1

rm symbols.c
python3 ../tools/gen_script.py kernel/myos symbols.c
find . -mindepth 1 ! -name 'symbols.c' -delete
cmake .. -DCMAKE_TOOLCHAIN_FILE=../x86_64-toolchain.cmake
make  #VERBOSE=1



#Check if valid Multiboot header
if grub-file --is-x86-multiboot2 kernel/myos; then
  echo multiboot confirmed
else
  echo the file is not multiboot
fi

mkdir -p isodir/boot/grub
cp kernel/myos isodir/boot/myos
cp ../grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o myos.iso isodir

#Cpu max for stack protectors hw number generator in hw_rand.asm
qemu-system-x86_64 -cdrom myos.iso -serial stdio
# qemu-system-x86_64 -cpu max -d cpu_reset,int -no-shutdown -no-reboot -cdrom myos.iso
#qemu-system-x86_64 -cpu max -kernel myos #Direct boot, no grub menu => Error loading uncompressed kernel without PVH ELF Note 

#For usb
#sudo dd if=myos.iso of=/dev/sdx && sync

#Objdump
#~/opt/cross/bin/x86_64-elf-objdump -M intel -d build/kernel/myos
#readelf -h *
