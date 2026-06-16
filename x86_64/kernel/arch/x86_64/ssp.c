#include <stdint.h>
#include <stdio.h> //for NULL
#include "vga.h"

//Smashing Stack protector

// Allocate the master guard key. 
// We give it a temporary default fallback value in case the CPU doesn't support RDRAND.
uintptr_t __stack_chk_guard = 0xDEADC0DEBEEFCAFE;

extern uint64_t read_hardware_random(void);
extern int is_address_readable(uintptr_t addr);


// For kernel symbol lookup table
typedef struct {
    uintptr_t address;
    const char *name;
} kernel_symbol_t;

// These are generated at compile time by the script and linked into our binary
extern const kernel_symbol_t kernel_symbol_table[];
extern const int kernel_symbol_count;

// Given a return address, find the function name it belongs to
__attribute__((no_stack_protector))
const char* lookup_symbol(uintptr_t addr) {
    if (kernel_symbol_count == 0) return "Unknown";

    int best_match = -1;

    // The symbol table is sorted by memory address
    for (int i = 0; i < kernel_symbol_count; i++) {
        if (kernel_symbol_table[i].address <= addr) {
            best_match = i;
        } else {
            // Since it's sorted, the moment the entry address exceeds our target,
            // the previous 'best_match' is our containing function frame!
            break; 
        }
    }

    if (best_match != -1) {
        return kernel_symbol_table[best_match].name;
    }
    return "Unknown Function";
}

// These are external symbols provided by the linker script.
// They don't hold values themselves; their MEMORY ADDRESSES are the values.
// extern uint8_t _kernel_start[];
// extern uint8_t _kernel_end[];
// uintptr_t low_limit  = (uintptr_t)_kernel_start;

__attribute__((no_stack_protector))
void serial_putc(char c) {
    // Wait for the serial port transmit buffer to be empty
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    // Send the character
    outb(0x3F8, c);
}

// A primitive to print a raw hex address without allocating stack buffers
__attribute__((no_stack_protector))
int serial_print_hex(uintptr_t val){
    static const char *chars = "0123456789ABCDEF";
    
    // Print "0x"
    serial_putc('0');
    serial_putc('x');
    
    // Extract hex digits from top (64-bit) to bottom
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (val >> i) & 0xF;
        serial_putc(chars[digit]);
    }
}
__attribute__((no_stack_protector))
int serial_print_string(const char *str) {
    if (!str) return;

    // Loop through the characters sequentially in memory until hitting the null terminator.
    // 'cursor' increments directly inside CPU registers, bypassing the stack.
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putc(str[i]);
    }
}

// The complete, completely isolated Stack Walker
struct stack_frame {
    struct stack_frame* next;
    uintptr_t return_address;
};

__attribute__((no_stack_protector))
void print_stack_trace(void) {
    struct stack_frame *frame; 
    
    // Grab the current frame anchor
    __asm__ __volatile__("mov %%rbp, %0" : "=r"(frame));

    int depth = 0;
    if (!is_address_readable((uintptr_t)frame)) {
        serial_print_hex(frame->return_address);
        serial_print_string(" : ");
        serial_print_string("Corrupted Stack, can't read further\n");
    }
    while (frame != NULL && depth < 10) {

        // If it passes both tests, it is safe to read the return address!
        serial_print_hex(frame->return_address);

        const char *func_name = lookup_symbol(frame->return_address);

        // Move to parent frame
        frame = frame->next;
        if (!is_address_readable((uintptr_t)frame)) {
            serial_print_string(" : ");
            serial_print_string("Corrupted Stack, can't read further\n");
            break; // Smashed garbage pointer detected safely! Stop looping.
        }

        serial_print_string(" : ");
        serial_print_string(func_name);
        serial_putc('\n');
        depth++;
    }
}


/**
 * Emergency Panic Trigger.
 * This must not use the stack protector flag during compilation!
 */
__attribute__((noreturn,no_stack_protector))
void __stack_chk_fail(void) {
    serial_print_string("######################################################\n");
    serial_print_string("############ PANIC: STACK SMASH DETECTED! ############\n");
    serial_print_string("######################################################\n");

    serial_print_string("Faulting instr : ");

    uintptr_t faulting_instruction = (uintptr_t)__builtin_return_address(0);
    serial_print_hex(faulting_instruction);
    serial_print_string("(");
    serial_print_string(lookup_symbol(faulting_instruction));
    serial_print_string(")");

    serial_putc('\n');

    serial_print_string("Stack trace : \n");
    print_stack_trace();
    
    serial_print_string("HALTING THE CPU");
    // 2. Safely lock down the CPU core permanently
    while (1) {
        __asm__ __volatile__ ("cli; hlt");
    }
}


uint64_t simple_fallback_rand(void) {
    static uint64_t seed = 0x1234567890ABCDEF;
    seed ^= (seed << 21);
    seed ^= (seed >> 35);
    seed ^= (seed << 4);
    return seed;
}

void kernel_init_stack_protector(void) {
    uint32_t eax, ebx, ecx, edx;

    // Query CPUID Leaf 1 to check processor features
    eax = 1;
    __asm__ __volatile__("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(eax));

    // Bit 30 of ECX tells us if RDRAND is supported by the physical silicon
    if (ecx & (1 << 30)) {
        __stack_chk_guard = (uintptr_t)read_hardware_random();
    } else {
        // Fallback safely to avoid a #UD exception on older processors
        __stack_chk_guard = (uintptr_t)simple_fallback_rand();
    }
}