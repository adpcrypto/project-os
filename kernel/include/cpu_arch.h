#ifndef _CPU_ARCH_H
#define _CPU_ARCH_H
#include <stdint.h>
__attribute__((no_stack_protector))
static inline void get_cpu_vendor_string_safe(char out_vendor[13]) {
    uint32_t reg_ebx, reg_edx, reg_ecx;

    __asm__ __volatile__ (
        "push %%rbx\n\t"           // Save 64-bit RBX
        "cpuid\n\t"
        "movl %%ebx, %0\n\t"       // Move lower 32-bits to our variable
        "pop %%rbx"                // Restore 64-bit RBX
        : "=r"(reg_ebx), "=d"(reg_edx), "=c"(reg_ecx)
        : "a"(0)
        : "memory" 
    );

    *(uint32_t*)&out_vendor[0] = reg_ebx;
    *(uint32_t*)&out_vendor[4] = reg_edx;
    *(uint32_t*)&out_vendor[8] = reg_ecx;
    out_vendor[12] = '\0';
}
#endif