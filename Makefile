# Makefile for Orion OS

CC = gcc
AS = gcc
LD = ld

BUILD_DIR = build
ISO_DIR = iso

KERNEL_OBJ = $(BUILD_DIR)/kernel.o
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

ALL: $(KERNEL_ELF)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(KERNEL_OBJ): | $(BUILD_DIR)
	@echo "Compiling kernel objects..."
	$(CC) -ffreestanding -c -g kernel/kmain.c -o $(KERNEL_OBJ)

$(KERNEL_ELF): $(KERNEL_OBJ) linker.ld kernel/arch/x86_64/boot/_start.asm
	@echo "Assembling entry..."
	nasm -f elf64 kernel/arch/x86_64/boot/_start.asm -o $(BUILD_DIR)/start.o
	@echo "Linking kernel ELF..."
	ld -T linker.ld -o $(KERNEL_ELF) $(BUILD_DIR)/start.o $(KERNEL_OBJ)

run: $(KERNEL_ELF)
	@echo "Launching QEMU with kernel ELF (serial on stdio)..."
	qemu-system-x86_64 -kernel $(KERNEL_ELF) -serial stdio

debug: $(KERNEL_ELF)
	@echo "Launching QEMU paused for GDB..."
	qemu-system-x86_64 -s -S -kernel $(KERNEL_ELF) -serial stdio

iso: $(KERNEL_ELF)
	@echo "Creating ISO (placeholder)..."
	mkdir -p $(ISO_DIR)
	cp $(KERNEL_ELF) $(ISO_DIR)/kernel.elf

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(ISO_DIR)

lint:
	@echo "Running lint checks..."
