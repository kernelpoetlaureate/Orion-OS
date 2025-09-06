; Multiboot starts us in 32-bit protected mode
bits 32

; The C function we will call
extern kmain

; Multiboot header (multiboot 2) so GRUB can recognize and load the kernel.
section .multiboot_header
header_start:
    dd 0xe85250d6                ; magic
    dd 0                         ; architecture (protected mode i386)
    dd header_end - header_start ; header length
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; required end tag
    dw 0 ; type
    dw 0 ; flags  
    dd 8 ; size
header_end:

; Stack for our kernel
section .bss
align 16
stack_bottom:
resb 4096 * 16 ; Reserve 16 pages for the stack
stack_top:

; Page tables for long mode
align 4096
pml4:
    resb 4096
pdpt:
    resb 4096
pd:
    resb 4096

; GDT for long mode
section .rodata
gdt64:
    dq 0 ; zero entry
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; code segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; The actual entry point of our kernel (32-bit)
section .text
global _start
_start:
    ; Set up the stack
    mov esp, stack_top
    
    ; Show we're starting
    mov dword [0xb8000], 0x2f532f53 ; 'SS' in white on green (Starting Setup)
    
    ; Set the Multiboot information pointer
    mov [multiboot_info_ptr], ebx
    
    ; Check if long mode is available
    call check_long_mode
    test eax, eax
    jz no_long_mode
    
    ; Set up page tables for long mode
    call setup_page_tables
    call enable_paging
    
    ; Load GDT
    lgdt [gdt64.pointer]
    
    ; Show we're about to jump
    mov dword [0xb8004], 0x2f4a2f4a ; 'JJ' (Jump to 64-bit)
    
    ; Jump to 64-bit code
    jmp gdt64.code:long_mode_start

no_long_mode:
    ; Show error if no long mode
    mov dword [0xb8000], 0x4f454f4e ; 'NE' in white on red (No Extension)
    cli
    hlt

check_long_mode:
    ; Check if CPUID is available
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid
    
    ; Check if extended CPUID is available
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    ; Check if long mode is available
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    
    mov eax, 1
    ret

.no_cpuid:
.no_long_mode:
    mov eax, 0
    ret

setup_page_tables:
    ; Clear page tables
    mov edi, pml4
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd
    mov edi, pml4
    
    ; Map first PML4 entry to PDPT
    mov eax, pdpt
    or eax, 0b11 ; present + writable
    mov [pml4], eax
    
    ; Map first PDPT entry to PD
    mov eax, pd
    or eax, 0b11 ; present + writable
    mov [pdpt], eax
    
    ; Map each PD entry to a 2MiB page
    mov ecx, 0
.map_pd_table:
    mov eax, 0x200000  ; 2MiB
    mul ecx
    or eax, 0b10000011 ; present + writable + huge page
    mov [pd + ecx * 8], eax
    
    inc ecx
    cmp ecx, 512
    jne .map_pd_table
    
    ret

enable_paging:
    ; Load PML4 into cr3
    mov eax, pml4
    mov cr3, eax
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ret

; 64-bit code starts here
bits 64
long_mode_start:
    ; Clear segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up 64-bit stack
    mov rsp, stack_top
    
    ; Show we're in 64-bit mode
    mov qword [0xb8008], 0x2f342f36 ; '64' in white on green
    
    ; Pass the Multiboot information structure pointer to kmain
    mov rdi, [multiboot_info_ptr]
    call kmain
    
    ; If kmain returns, halt the CPU
    cli
.hang:
    hlt
    jmp .hang

; Define the Multiboot information pointer
section .data
multiboot_info_ptr: dq 0


