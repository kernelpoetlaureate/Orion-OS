; Minimal 64-bit entry compatible with Limine or GRUB when they transfer control to long mode.
; Sets up a stack and calls kmain.

    .global _start
    .text
_start:
    ; Clear direction flag
    cld

    ; Set up stack. We use a small stack in .bss; the linker will provide _stack_top.
    lea _stack_top(%rip), %rsp

    ; Call C entry
    call kmain

halt_loop:
    hlt
    jmp halt_loop

; Reserve stack space symbol (defined in linker script as ORIGIN + size)
