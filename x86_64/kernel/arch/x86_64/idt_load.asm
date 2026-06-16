bits 64
global idt_load_hardware

; 1. The Hardware Loader
idt_load_hardware:
    lidt [rdi]            ; RDI holds the address of idt_ptr via C calling convention
    sti                   ; Critically important: Re-enable hardware interrupts on the CPU!
    ret