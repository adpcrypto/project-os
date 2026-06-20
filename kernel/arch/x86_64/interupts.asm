bits 64
section .text

; External reference to our single C router switchboard
extern universal_c_router

; --- The Intel Syntax Macro Matrix ---
; This macro builds individual entry stubs that push parameters onto the stack
%macro INTERRUPT_ENTRY_NO_ERROR 1
global interrupt_entry_%1
interrupt_entry_%1:
    push 0      ; Push a dummy error code to keep the stack frame uniform - Stack frame is defined in C code
    push %1     ; Push the unique Vector Number (passed to C as state->vector_number)
    jmp common_interrupt_handler
%endmacro

%macro INTERRUPT_ENTRY_WITH_ERROR 1
global interrupt_entry_%1
interrupt_entry_%1:
    ; The CPU hardware has already automatically pushed the real error code here!
    push %1     ; Push the unique Vector Number
    jmp common_interrupt_handler
%endmacro

; --- Define the System Entry Point Matrix ---
INTERRUPT_ENTRY_NO_ERROR   0   ; Divide by Zero Fault (#DE)
INTERRUPT_ENTRY_NO_ERROR   1   ; Debug Exception
INTERRUPT_ENTRY_WITH_ERROR 13  ; General Protection Fault (#GP)
INTERRUPT_ENTRY_WITH_ERROR 14  ; Page Fault (#PF)
INTERRUPT_ENTRY_NO_ERROR   32  ; PIT Timer (Hardware IRQ 0)
INTERRUPT_ENTRY_NO_ERROR   33  ; PS/2 Keyboard (Hardware IRQ 1)

; --- The Centralized Assembly Handoff Routing Station ---
common_interrupt_handler:
    ; 1. Preserve the exact CPU execution context state (Intel Syntax: push register)
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; 2. Pass the Stack Pointer into the first argument register
    ; System V AMD64 Calling Convention: RDI = Argument 1
    mov rdi, rsp
    
    ; 3. Safely cross the bridge into your C framework
    call universal_c_router

    ; 4. Restore the CPU execution state in exact reverse order
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; 5. Clean up the custom Vector Number and Error Code parameters off the stack
    add rsp, 16
    
    ; 6. Atomically restore RFLAGS and jump back to the main thread
    iretq