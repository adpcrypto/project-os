cmake_minimum_required(VERSION 3.20)

# Enable C and Assembly (ASM) languages for this project
project(MyCustomOS LANGUAGES C ASM_NASM)

# Ensure a toolchain file is being used for cross-compilation
if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "You must specify a toolchain file! Use: cmake --toolchain=path/to/file.cmake")
endif()

# Define our build-time Sysroot inside the build directory
set(PARENT_DIR "${CMAKE_SOURCE_DIR}")
set(SYSROOT_DIR "${CMAKE_BINARY_DIR}/sysroot")
set(SYSROOT_TARGET_DIR "${SYSROOT_DIR}/usr")

set(KERNEL_FLAGS
    "-ffreestanding " 
    "-O2 "
    "-Wall "
    "-Wextra "
    "-mno-sse "                 # Shouldnt use SIMD (single instr. multiple data), kinda vector   
    "-mno-sse2 "                # Shouldnt use SIMD (single instr. multiple data), kinda vector             
    "-fstack-protector-strong " # SSP is implemetned
)
string(REPLACE ";" " " CMAKE_C_FLAGS "${KERNEL_FLAGS}")


# Tell the compiler to treat the sysroot include directory as a SYSTEM path (<>)
include_directories(SYSTEM "${SYSROOT_TARGET_DIR}/include")

# Build libc first (so kernel can link against it later), then the kernel
add_subdirectory(libc)
add_subdirectory(kernel)