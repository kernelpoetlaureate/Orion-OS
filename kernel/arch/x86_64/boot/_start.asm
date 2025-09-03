; Set the architecture to 64-bit
bits 64

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

    ; required tags
    dw 0 ; end tag
    dw 0 ; flags
    dd 8 ; size
header_end:

; Limine requires a stack, but it provides one for us. We just need to declare this
; symbol so Limine can tell us its size and location. It's good practice to define
; our own later, but for now this is fine.
section .bss
align 16
stack_bottom:
resb 4096 * 16 ; Reserve 16 pages for the stack
stack_top:

; The Limine boot request. This tells Limine where to start our kernel.
section .limine_req
align 8
limine_request:
    ; Limine common magic and entry point request id (from limine.h)
    dq 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b  ; LIMINE_COMMON_MAGIC
    dq 0x13d86c035a1cd3e1, 0x2b0caa89d8f3026a  ; LIMINE_ENTRY_POINT_REQUEST ID
    dq 0                                    ; Revision

    ; Response pointer (Limine will fill this)
    dq 0

    ; Entry point request fields
    dq 0                                    ; placeholder for response pointer (unused)
    dq _start                               ; The entry point for our kernel

    ; End of requests (null tag)
    dq 0, 0

; The actual entry point of our kernel
section .text
global _start
_start:
    ; Limine guarantees that the stack pointer (rsp) is already set up.
    ; For now, we can trust it. We will set our own stack pointer later on.
    mov rsp, stack_top  ; We can uncomment this if we want to use our own stack

    ; Limine passes a pointer to the boot info structure in the rdi register.
    ; We can save this for later, but for now, we don't need it.

    ; Align the stack to 16 bytes before calling kmain
    sub rsp, 8

    ; Call the C kernel's main function
    call kmain

    ; If kmain ever returns (it shouldn't), halt the system.
halt:
    cli ; Clear interrupts
    hlt ; Halt the CPU
    jmp halt
