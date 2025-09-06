/* Linker script for Orion OS with explicit program headers to avoid RWX PT_LOAD */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

/* Define program headers: one for RX code, one for RW data */
PHDRS {
  text PT_LOAD FLAGS(5); /* PF_R | PF_X */
  data PT_LOAD FLAGS(6); /* PF_R | PF_W */
}
SECTIONS
{
  /* Place file layout near start of file so p_offset stays small (GRUB can read it).
    Virtual mapping can be adjusted later if needed; using 0x1000 is fine for testing. */
  . = 0x1000;

  /* Put multiboot header early so GRUB finds it within the first 8KB of the file */
  .multiboot_header : { *(.multiboot_header) } :text

  /* Code + rodata in the RX segment */
  .text : { *(.text*) } :text

  .rodata : { *(.rodata*) } :text

  /* Data in a separate RW segment */
  .data : { *(.data*) } :data

  .bss (NOLOAD) : { 
    _bss_start = .;
    *(.bss*)
    *(COMMON)
    _bss_end = .;
  } :data

  /* Provide a symbol for stack top — 16KB stack */
  . = ALIGN(8);
  _stack_bottom = .;
  .stack (NOLOAD) : {
    . = . + 16384;
  } :data
  _stack_top = .;

  /DISCARD/ : { *(.eh_frame*) }
}
