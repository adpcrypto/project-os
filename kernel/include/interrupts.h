#ifndef _INTR_H
#define _INTR_H
#include "ssp.h"
void init_idt(void);
void flush_keyboard_controller(void);

__attribute__((no_stack_protector))
void generic_panic_c(void) {
    serial_print_string("\n!!! UNHANDLED HARDWARE EXCEPTION TRAP !!!\n");
}

#endif