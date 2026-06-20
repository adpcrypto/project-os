global generic_exception_stub
extern generic_panic_c
global keyboard_isr_stub
extern keyboard_handler_c ; Your C function that reads port 0x60

generic_exception_stub:
    cli                         ; Turn off interrupts immediately
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    call generic_panic_c        ; Jump to a diagnostic print screen
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    iretq 

; 2. The Context-Saving Keyboard Interrupt Stub
keyboard_isr_stub:
    ; Push all registers that your C code might modify to avoid background corruption
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    call keyboard_handler_c   ; Safely execute your C keyboard logic

    ; Pop everything back in precise reverse order
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    iretq                 ; Special Return: Pops CS, RIP, RFLAGS, SS, and RSP simultaneously