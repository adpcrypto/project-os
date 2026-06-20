# Target Operating System Configuration
set(CMAKE_SYSTEM_NAME Generic) # "Generic" tells CMake we are building for bare-metal
set(CMAKE_SYSTEM_PROCESSOR x86_64) # Or i686 depending on your compiler

# Define the Cross-Compiler Paths
set(TOOLCHAIN_PREFIX "$ENV{HOME}/opt/cross/bin/x86_64-elf-")

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")

#Doesnt build a full binary, so that it doesnt link to libc
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")