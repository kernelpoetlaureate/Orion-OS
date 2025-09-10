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
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    dw 0 ; type
    dw 0 ; flags  
    dd 8 ; size
header_end:

; Stack for our kernel
section .bss
align 16
stack:
resb 4096 * 16

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
    dq 0
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; The actual entry point of our kernel (32-bit)
section .text
global _start
_start:
    mov esp, stack + 4096 * 16
    call setup_page_tables
    call enable_paging
    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

setup_page_tables:
    xor eax, eax
    mov ecx, 4096
    mov edi, pml4
    rep stosd
    mov eax, pdpt
    or eax, 0b11
    mov [pml4], eax
    mov eax, pd
    or eax, 0b11
    mov [pdpt], eax
    mov ecx, 0
.map_pd_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [pd + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_pd_table
    ret

enable_paging:
    mov eax, pml4
    mov cr3, eax
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

bits 64
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov rsp, stack + 4096 * 16
    mov rdi, [multiboot_info_ptr]
    call kmain
    cli
.hang:
    hlt
    jmp .hang

section .data
multiboot_info_ptr: dq 0


