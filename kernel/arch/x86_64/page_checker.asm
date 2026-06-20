section .text
bits 64
global is_address_readable

; -----------------------------------------------------------------------------
; int is_address_readable(uintptr_t addr);
; Input:  RDI = Virtual memory address to check (System V AMD64 ABI)
; Output: RAX = 1 if safe to read, 0 if reading it will trigger a Page Fault
; -----------------------------------------------------------------------------
is_address_readable:
    push rbp
    mov rbp, rsp

    ; 1. Grab physical root address of PML4 from CR3 register
    mov rax, cr3
    mov r8, 0xFFFFFFFFFFFFF000  ; Bitmask to clear control flags (lower 12 bits)
    and rax, r8                 ; RAX = Raw physical pointer to PML4 root

    ; 2. Check Level 4: PML4 Table
    mov rcx, rdi
    shr rcx, 39                 ; Extract bits 39-47
    and rcx, 0x1FF              ; Filter down to a 9-bit array index (0-511)
    
    mov rdx, [rax + rcx*8]      ; Fetch the 8-byte entry from the PML4 table
    test rdx, 1                 ; Is Bit 0 (PTE_PRESENT) set?
    jz .not_readable            ; If 0, the entry is empty/invalid!

    ; 3. Check Level 3: PDPT Table
    and rdx, r8                 ; Isolate the physical address pointing to PDPT
    mov rcx, rdi
    shr rcx, 30                 ; Extract bits 30-38
    and rcx, 0x1FF              ; Filter down to a 9-bit array index
    
    mov rax, [rdx + rcx*8]      ; Fetch the 8-byte entry from the PDPT table
    test rax, 1                 ; Is Bit 0 (PTE_PRESENT) set?
    jz .not_readable

    ; --- HUGE PAGE CROSSROAD ---
    test rax, 0x80              ; Is Bit 7 (Page Size) set to 1?
    jnz .is_readable            ; If yes, this is a 1GB Huge Page mapping! Stop here.

    ; 4. Check Level 2: Page Directory (Only processed if not a 1GB map)
    and rax, r8                 ; Isolate physical address pointing to Page Directory
    mov rcx, rdi
    shr rcx, 21                 ; Extract bits 21-29
    and rcx, 0x1FF              ; Filter down to a 9-bit index
    
    mov rdx, [rax + rcx*8]      ; Fetch the entry
    test rdx, 1                 ; Is Present?
    jz .not_readable
    test rdx, 0x80              ; Is Bit 7 set here? (2MB Huge Page)
    jnz .is_readable            ; If yes, this is a valid 2MB entry! Stop here.

    ; 5. Check Level 1: Standard Page Table (Only reached for explicit 4KB mappings)
    and rdx, r8                 ; Isolate physical address pointing to Page Table
    mov rcx, rdi
    shr rcx, 12                 ; Extract bits 12-20
    and rcx, 0x1FF              ; Filter down to a 9-bit index
    
    mov rax, [rdx + rcx*8]      ; Fetch individual page entry
    test rax, 1                 ; Is Present?
    jz .not_readable

.is_readable:
    mov rax, 1                  ; Hardware verifies entry is active. Safe to read!
    pop rbp
    ret

.not_readable:
    mov rax, 0                  ; Entry is completely unmapped or dangerous.
    pop rbp
    ret