bits 32

section .multiboot2
align 8
multiboot_start:
    dd 0xE85250D6                                           ; Magic number for multiboot 2
    dd 0                                                    ; Architecture 0 (protected mode i386)
    dd multiboot_end - multiboot_start                      ; Header length        
    dd -(0xE85250D6 + 0 + (multiboot_end - multiboot_start)); Checksum

    ; Required end tag
    dw 0
    dw 0
    dd 8
multiboot_end:

section .bootstrap_stack nobits
align 16
stack_bottom:
    resb 16384      ; 16KB of stack space TODOis it bit or byte?
stack_top:

section .bss
align 4096
;Allocate memory for 4-level page tables (4KB each)
pml4_table:
    resb 4096
pdpt_table:
    resb 4096
page_directory:
    resb 4096

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top

    ; 1. Verify CPUID and long support mode
    ; Skipping detailed error checks

    ; 2. Setup 4 level paging (Identity map the first 2MB)
    ; Point first entry of pml4 to pdpt
    mov eax, pdpt_table     ; TODO why are we using ax instead of rax
    or eax, 0b11            ; Present + writable flags
    mov [pml4_table], eax

    ; Point first entry of pdpt table to page directory
    mov eax, page_directory
    or eax, 0b11
    mov [pdpt_table], eax

    ; Point first entry of page directory to a huge 2MB page
    ; Bit 7 (0x80) makes this a 2MB page instead of pointing to a page table
    mov eax, 0x000000 | 0b10000011      ; Present + writable + huge page
    mov [page_directory], eax

    ; 3. Load page table pointer into cr3
    mov eax, pml4_table
    mov cr3, eax

    ; 4. Enable PAE - Physical Address Extension
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 5. Switch to long mode via EFER MSR
    mov ecx, 0xC0000080     ; EFER MSR register address
    rdmsr
    or eax, 1 << 8          ; Set long mode enable (LME) bit
    wrmsr

    ; 6. Enable Paging
    mov eax, cr0
    or eax, 1 << 31         ; Set paging (PG) bit 
    mov cr0, eax

    ; Load 64-bit global desciptor table
    lgdt [gdt64.pointer]

    ; Far jump into 64-bit code segment to flush pipeline
    jmp gdt64.code_segment:init_64bit

bits 64
init_64bit:
    ; Update segment registers for 64-bit mode
    mov ax, gdt64.data_segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to your 64-bit C kernel
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang

section .rodata
; 64-bit Global Descriptor Table Setup
gdt64:
    dq 0        ; Null descriptor
.code_segment: equ $ - gdt64
    ; Code descriptor: Access=1, Readable=1, Conforming=0
    ;                  Code=1, LongMode=1
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
.data_segment: equ $ - gdt64
    ; Data descriptor: Access=1, Writable=1, Code=0
    dq (1<<41) | (1<<44) | (1<<47)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
