project(k_libc C)

# Define the library

file(GLOB LIBC_SOURCES 
    "stdio/*.c"
    "stdlib/*.c"
    "string/*.c"
)
add_library(k_libc STATIC ${LIBC_SOURCES})

# Strip standard libraries from linking, change name to 'c' to mimic libc
set_target_properties(k_libc PROPERTIES 
    OUTPUT_NAME "c"
    LINKER_LANGUAGE C
)

target_compile_definitions(k_libc PRIVATE __is_libk)

# --- The Magic Step for Meaty Skeleton ---
# Copy public libc headers to the sysroot at build time
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
     DESTINATION "${SYSROOT_TARGET_DIR}/include"
)