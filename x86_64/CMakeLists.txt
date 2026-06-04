cmake_minimum_required(VERSION 3.16)

# Enable C and Assembly (ASM) languages for this project
project(MyCustomOS LANGUAGES C ASM_NASM)

# Ensure C compiler knows we are writing bare-metal, freestanding code
set(CMAKE_C_FLAGS
    "-ffreestanding \
    -O2 \
    -Wall \
    -Wextra"
)


set(CMAKE_ASM_FLAGS "-f elf64")

# Define your source files (matching the Bare Bones layout)
set(KERNEL_SOURCES
    boot.asm
    kernel.c
)

# Create the final kernel executable
add_executable(myos ${KERNEL_SOURCES})

target_link_options(myos PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    -nostdlib
    -lgcc
)