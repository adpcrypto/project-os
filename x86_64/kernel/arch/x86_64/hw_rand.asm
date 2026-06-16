global read_hardware_random

section .text

; ------------------------------------------------------------------------
; uint64_t read_hardware_random(void);
; Returns a true 64-bit random number in RAX.
; If the hardware generator is temporarily empty, it loops until it succeeds.
; ------------------------------------------------------------------------
read_hardware_random:
.retry:
    rdrand rax      ; Query the hardware entropy engine, results drop into RAX
    jnc .retry      ; Carry Flag (CF) is cleared if the random value wasn't ready. 
                    ; If CF = 0, loop back and try again immediately.
    ret             ; Return to C with the true random number sitting in RAX