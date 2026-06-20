#ifndef _KERNEL_SSP_H
#define _KERNEL_SSP_H

#include <stdint.h>

void kernel_init_stack_protector(void);
void serial_putc(char c);
int serial_print_hex(uintptr_t val);
int serial_print_string(const char *str);
void print_stack_trace(void);

#endif