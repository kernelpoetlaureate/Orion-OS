; Minimal 64-bit entry compatible with Limine or GRUB when they transfer control to long mode.
; Sets up a stack and calls kmain.

global _start
extern _stack_top
extern kmain

section .multiboot_header
align 4
dd 0x1BADB002            ; Magic number

dd 0x0                   ; Flags

dd -(0x1BADB002 + 0x0)   ; Checksum

    .text
_start:
    ; Clear direction flag
    cld

    ; Set up stack. We use a small stack in .bss; the linker will provide _stack_top.
    mov rsp, _stack_top

    ; Call C entry
    call kmain

halt_loop:
    hlt
    jmp halt_loop

; Reserve stack space symbol (defined in linker script as ORIGIN + size)
