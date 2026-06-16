// Simple text mode VGA pointer (64-bit address space)
#include <stdio.h>

#include <tty.h>
#include <string.h>
#include "ssp.h"

__attribute__((noinline))
void secure_looking_copy(char *src, int length) {
    char stack_buffer[16];

    memcpy(stack_buffer, src, length);
}

// The trigger function that forces the dynamic payload injection
__attribute__((noinline))
void trigger_stack_smash(void) {

    char malicious_payload[64];
    for (int i = 0; i < 64; i++) {
        malicious_payload[i] = 'X';
    }

    //INSECURE
    secure_looking_copy(malicious_payload, 64);
}

__attribute__((no_stack_protector)) //Kernel entry should't cehck its own ssp
void kernel_main(void){

    terminal_initialize();

    serial_print_string("Kernel Entrance\n");
    terminal_initialize();
    terminal_writestring("Terminal Initialized\n");

    kernel_init_stack_protector();
    terminal_writestring("Stack protector initialized.\n");

    // terminal_writestring("Triggering intentional stack smash test...\n");
    // trigger_stack_smash();

    // terminal_writestring("ERROR: If you see this, the stack protector FAILED!");
}



