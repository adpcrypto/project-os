project(my_os_kernel C ASM_NASM)

# 1. Copy public kernel headers to the sysroot so <kernel/tty.h> is valid
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
     DESTINATION "${SYSROOT_TARGET_DIR}/include"
)

set(CMAKE_ASM_FLAGS "-f elf64")

set(ARCH "x86_64") 

# option(ENABLE_EMBEDDED_SYMBOLS "Include the auto-generated symbol table file" OFF)

#Even while running normally, we will link against a blank symbolfile
set(KERNEL_SRCS
    kernel.c
    arch/${ARCH}/tty.c
    arch/${ARCH}/ssp.c
    arch/${ARCH}/boot.asm
    arch/${ARCH}/io.asm
    arch/${ARCH}/hw_rand.asm
    arch/${ARCH}/page_checker.asm
    ../build/symbols.c
)

# if(ENABLE_EMBEDDED_SYMBOLS)
#     message(STATUS "--> Embedded symbols enabled! Including symbols.c in build.")
#     list(APPEND KERNEL_SRCS ../build/symbols.c)
#     # You can also pass a macro definition to your C code if needed

#     
#     #add_compile_definitions(SYMBOL_TABLE=1)
# else()
#     message(STATUS "--> Embedded symbols disabled.")
# endif()

# 3. Define the kernel executable (or elf binary)
add_executable(myos ${KERNEL_SRCS})

# Set freestanding link options
target_link_options(myos PRIVATE
    -ffreestanding
    -nostdlib
    -T ${PARENT_DIR}/linker.ld
)

# 4. Handle Private Headers ("vga.h")
# To ensure tty.c can find "vga.h" using double quotes, we add the local arch folder 
# to the include path for the kernel target ONLY.
# target_include_directories(myos PRIVATE "arch/${ARCH}")

# 5. Link against our custom freestanding libc
target_link_libraries(myos PRIVATE k_libc)