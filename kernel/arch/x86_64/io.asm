global outb
global inb
global io_wait

section .text

; ------------------------------------------------------------------------
; void outb(uint16_t port, uint8_t value);
; C Compiler places:
;   port  (1st arg) -> di  (lower 16-bits of rdi)
;   value (2nd arg) -> sil (lower 8-bits of rsi)
; ------------------------------------------------------------------------
outb:
    mov dx, di      ; Move the 16-bit port number into DX
    mov al, sil     ; Move the 8-bit value into AL
    out dx, al      ; Send byte in AL to port in DX
    ret

; ------------------------------------------------------------------------
; uint8_t inb(uint16_t port);
; C Compiler places:
;   port  (1st arg) -> di  (lower 16-bits of rdi)
; C Expects the return value to be placed inside the AL register
; ------------------------------------------------------------------------
inb:
    mov dx, di      ; Move the 16-bit port number into DX
    in al, dx       ; Read byte from port DX into AL
    ret             ; Return to C (C automatically reads AL for the result)

; --- io_wait ---
; Forces a tiny ~1 microsecond delay for slow motherboard bus synchronization
io_wait:
    mov al, 0
    out 0x80, al    ; Write a dummy zero byte to the safe POST diagnostic port
    ret