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

__attribute__((no_stack_protector)) 
void trigger_divide_by_zero(void) {
    uint64_t rax_val = 5;
    uint64_t rcx_val = 0;

    // Force a raw hardware 64-bit division: RAX divided by RCX
    __asm__ volatile(
        ".intel_syntax noprefix\n\t"
        "xor rdx, rdx\n\t"  // Clean Intel Syntax! No double percent signs needed.
        "div rcx\n\t"       // Explicitly divide by the rcx register
        ".att_syntax\n\t"   // Safely switch back so GCC doesn't break later files
        : "=a"(rax_val)
        : "c"(rcx_val)      // "c" forces the compiler to put rcx_val into RCX explicitly
        : "rdx"
    );
}

__attribute__((no_stack_protector)) //Kernel entry should't cehck its own ssp
void kernel_main(void){

    terminal_initialize();

    serial_print_string("Kernel Entrance\n");
    terminal_initialize();
    terminal_writestring("Terminal Initialized\n");

    flush_keyboard_controller();
    init_idt();
    terminal_writestring("IDT Initialized\n");

    // Manually fire Interrupt 33 (0x21) using pure inline assembly
    __asm__ __volatile__("int $0x21");

    print_cpu_arch();

    kernel_init_stack_protector();
    terminal_writestring("Stack protector initialized.\n");

    // trigger_divide_by_zero();
    // terminal_writestring("Triggering intentional stack smash test...\n");
    // trigger_stack_smash();

    // terminal_writestring("ERROR: If you see this, the stack protector FAILED!");

    while (1) {
        // 'hlt' puts the CPU to sleep until an interrupt arrives.
        // When the interrupt finishes, it resumes here and loops back to hlt.
        __asm__ __volatile__ ("hlt"); 
    }
}



