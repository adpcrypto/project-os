#include <stdint.h>
#include "ssp.h"

// Import assembly macro symbols and the flush command
extern void idt_flush(uintptr_t idt_reg_ptr);
extern void interrupt_entry_0(void);
extern void interrupt_entry_13(void);
extern void interrupt_entry_14(void);
extern void interrupt_entry_32(void);
extern void interrupt_entry_33(void);

// Guard against stack protector issues in early boot structures
__attribute__((no_stack_protector))

// Define a single 64-bit IDT entry (16 bytes total)
typedef struct {
    uint16_t isr_low;   // The lower 16 bits of the ISR's address
    uint16_t kernel_cs; // The GDT selector for kernel code (usually 0x08)
    uint8_t  ist;       // Interrupt Stack Table offset (set to 0 if unused)
    uint8_t  attributes;// Type and attributes (e.g., Present, Ring 0, Interrupt Gate)
    uint16_t isr_mid;   // The middle 16 bits of the ISR's address
    uint32_t isr_high;  // The higher 32 bits of the ISR's address
    uint32_t reserved;  // Set to 0
} __attribute__((packed)) idt_entry_t;

// Sequential layout of the stack frame matching our assembly push sequence
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector_number; 
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss; 
} __attribute__((packed)) exception_state_t;

// The IDT Pointer structure that the 'lidt' assembly instruction demands
typedef struct {
    uint16_t limit;     // Size of IDT array minus 1 byte
    uint64_t base;      // The absolute 64-bit address of the IDT array
} __attribute__((packed)) idt_pointer_t;

// Allocate the alignment-safe array of 256 interrupt tracks
__attribute__((aligned(16))) static idt_entry_t idt[256];
static idt_pointer_t idt_ptr;


extern void keyboard_isr_stub(void);
extern void generic_exception_stub(void);
extern void idt_load_hardware(uintptr_t pointer_address);

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t attributes) {
    uintptr_t addr = (uintptr_t)isr;

    idt[vector].isr_low   = addr & 0xFFFF;
    idt[vector].kernel_cs = 0x08; // Points to your GDT Code Segment descriptor
    idt[vector].ist       = 0;
    idt[vector].attributes = attributes;
    idt[vector].isr_mid   = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high  = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved  = 0;
}

__attribute__((no_stack_protector))
void pic_init(void) {
    //By defu
    // ICW1: Start initialization chain
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();

    // ICW2: Set vector offsets (Hardware IRQs -> IDT entries 32-47)
    outb(0x21, 32);   // Master PIC starts at IDT 32 (Keyboard IRQ 1 = IDT 33)
    outb(0xA1, 40);   // Slave PIC starts at IDT 40

    // ICW3: Wire up Master and Slave cascade lines
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();

    // ICW4: Set up 8086 environment mode
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    // Unmask IRQ 1 (Keyboard) on Master PIC. 0xFD = 11111101b.
    // (Bit 1 is Keyboard, Bit 2 must stay 0 to allow Slave cascade routing)
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF); // Mask everything on Slave PIC
}
    
void flush_keyboard_controller(void) {
    // Read the keyboard status register (Port 0x64)
    // Bit 0 (0x01) tells us if there is data waiting to be read in Port 0x60
    // We loop until the status register says the buffer is completely empty.
    int timeout = 100000;
    while ((inb(0x64) & 0x01) && timeout > 0) {
        inb(0x60);  // Throw away the stuck byte
        io_wait();  // Give the bus a microsecond to breathe
        timeout--;
    }
}

// --- Centralized Routing Station ---
void universal_c_router(exception_state_t* state) {
    switch (state->vector_number) {
        case 0:
            serial_print_string("CRITICAL: Division by Zero caught.\n");
            print_stack_trace();
            while(1) { __asm__ volatile("cli; hlt"); }
            break;

        case 13:
            serial_print_string("CRITICAL: General Protection Fault. Error Code: ");
            serial_print_hex(state->error_code);
            serial_putc('\n');
            print_stack_trace();
            while(1) { __asm__ volatile("cli; hlt"); }
            break;

        case 14:
            serial_print_string("CRITICAL: Page Fault. Memory Violating RIP Address: ");
            serial_print_hex(state->rip);
            serial_putc('\n');
            while(1) { __asm__ volatile("cli; hlt"); }
            break;

        case 32:
            outb(0x20, 0x20); // Acknowledge Timer
            break;

        case 33:
            // serial_print_string("Key press: ");
            keyboard_handler_c();
            break;
    }
}

void init_idt(void) {
    pic_init();
    
    // 1. Clear out all 256 descriptors initially
    for (int i = 0; i < 256; i++) {
        idt_set_descriptor(i, 0, 0); 
    }

    // 2. Safely configure ONLY our requested vectors
    idt_set_descriptor(0,  interrupt_entry_0,  0x8E); // No Error: Divide-by-Zero
    idt_set_descriptor(13, interrupt_entry_13, 0x8E); // With Error: GP Fault
    idt_set_descriptor(14, interrupt_entry_14, 0x8E); // With Error: Page Fault
    idt_set_descriptor(32, interrupt_entry_32, 0x8E); // No Error: Timer (Keep PIC happy)
    idt_set_descriptor(33, interrupt_entry_33, 0x8E); // No Error: Keyboard

    // 3. Set up the IDT Pointer configuration
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base  = (uint64_t)&idt;

    // 4. Call the assembly wrapper to load the registers into the CPU core
    idt_load_hardware((uintptr_t)&idt_ptr);
}