# .gdbinit helper for kernel debugging
# Usage:
# 1) Start QEMU with: qemu-system-x86_64 -s -S -kernel build/orion.elf
# 2) In another terminal run: gdb build/orion.elf
# 3) Inside gdb: source .gdbinit
#    Then: target remote :1234
#    If your kernel is linked to a specific load address, use `add-symbol-file` with the lma

set architecture i386:x86-64
set pagination off

define load-symbols
  # Usage: load-symbols <elf-file> <load-address>
  if $argc < 2
    printf "Usage: load-symbols <elf-file> <load-address>\n"
  else
    add-symbol-file $arg0 $arg1
    printf "Added symbols from %s at %s\n", $arg0, $arg1
  end
end

# Convenience: break at main or kmain
break kmain
