#include <stdint.h>
#include "ssp.h"
#include "pic_def.h"

// Import assembly macro symbols and the flush command
extern void idt_flush(uintptr_t idt_reg_ptr);
extern void interrupt_entry_0(void);
extern void interrupt_entry_13(void);
extern void interrupt_entry_14(void);
extern void interrupt_entry_32(void);
extern void interrupt_entry_33(void);

__volatile__ uint64_t system_ticks = 0;

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
void pic_init(int offset1, int offset2) {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 1 << CASCADE_IRQ);        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

    //IRQs are unasked using pic_unmask_irq()
}
    
void pic_unmask_irq(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = 0x21; // Master PIC
    } else {
        port = 0xA1; // Slave PIC
        irq_line -= 8;
    }
    
    // Read current mask -> clear the bit -> write it back
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);
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
            // while(1) { __asm__ volatile("cli; hlt"); }
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
            system_ticks++;
            // serial_print_string("Ticks:");
            // serial_print_hex(system_ticks);
            // serial_putc('\n');
            outb(0x20, 0x20); // Acknowledge Timer
            break;

        case 33:
            // serial_print_string("Key press: ");
            keyboard_handler_c();
            break;
    }
}

void init_idt(void) {
    pic_init(32,40);
    pic_unmask_irq(0); //PIT
    pic_unmask_irq(1); //Keyboard
    
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