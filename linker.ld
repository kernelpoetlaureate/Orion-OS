/* Minimal linker script for Orion OS - higher-half kernel layout example */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
  /* Physical load address for the kernel: 0x100000 (1 MiB) */
  . = 0x100000;

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss (NOLOAD) : {
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  }

  /* Provide a symbol for stack top — 16KB stack */
  . = ALIGN(8);
  _stack_bottom = .;
  .stack (NOLOAD) : {
    . = . + 16384;
  }
  _stack_top = .;

  /DISCARD/ : { *(.eh_frame*) }
}
