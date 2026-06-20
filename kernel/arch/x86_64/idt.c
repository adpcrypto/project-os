#include <stdint.h>

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

void init_idt(void) {

    pic_init();
    
    for (int i = 0; i < 256; i++) {
        idt_set_descriptor(i, generic_exception_stub, 0x8E);
    }
    idt_set_descriptor(33, keyboard_isr_stub, 0x8E);

    // Setup the hardware layout pointer
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base  = (uint64_t)&idt;

    // Call assembly to physically load the pointer into the CPU core
    idt_load_hardware((uintptr_t)&idt_ptr);
}