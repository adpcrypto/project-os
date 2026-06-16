// Simple text mode VGA pointer (64-bit address space)
#include <stdio.h>

#include <tty.h>
#include <string.h>
#include "ssp.h"
#include "cpu_arch.h"
#include "interrupts.h"

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

void print_cpu_arch(void){
    char vendor[13];
    get_cpu_vendor_string_safe(vendor);

    terminal_writestring("Detected CPU/Hypervisor: ");
    terminal_writestring(vendor);
    terminal_writestring("\n");   
}

__attribute__((no_stack_protector)) //Kernel entry should't cehck its own ssp
void kernel_main(void){

    terminal_initialize();

    serial_print_string("Kernel Entrance\n");
    terminal_initialize();
    terminal_writestring("Terminal Initialized\n");

    // flush_keyboard_controller();
    init_idt();
    terminal_writestring("IDT Initialized\n");

    // Manually fire Interrupt 33 (0x21) using pure inline assembly
    __asm__ __volatile__("int $0x21");

    print_cpu_arch();

    kernel_init_stack_protector();
    terminal_writestring("Stack protector initialized.\n");

    // terminal_writestring("Triggering intentional stack smash test...\n");
    // trigger_stack_smash();

    // terminal_writestring("ERROR: If you see this, the stack protector FAILED!");

    while (1) {
        // 'hlt' puts the CPU to sleep until an interrupt arrives.
        // When the interrupt finishes, it resumes here and loops back to hlt.
        __asm__ __volatile__ ("hlt"); 
    }
}



