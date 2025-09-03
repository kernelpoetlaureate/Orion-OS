# Makefile for Orion OS

CC = gcc
AS = gcc
LD = ld

BUILD_DIR = build
ISO_DIR = iso

KERNEL_OBJ = $(BUILD_DIR)/kernel.o
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

DRIVER_OBJS = $(BUILD_DIR)/vga.o $(BUILD_DIR)/serial.o
LIB_OBJS = $(BUILD_DIR)/printf.o

ALL: $(KERNEL_ELF)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(KERNEL_OBJ): | $(BUILD_DIR)
	@echo "Compiling kernel objects..."
	$(CC) -ffreestanding -c -g kernel/kmain.c -o $(KERNEL_OBJ)

$(BUILD_DIR)/vga.o: kernel/drivers/vga.c | $(BUILD_DIR)
	$(CC) -ffreestanding -c -g kernel/drivers/vga.c -o $(BUILD_DIR)/vga.o

$(BUILD_DIR)/serial.o: kernel/drivers/serial.c | $(BUILD_DIR)
	$(CC) -ffreestanding -c -g kernel/drivers/serial.c -o $(BUILD_DIR)/serial.o

$(BUILD_DIR)/printf.o: kernel/lib/printf.c | $(BUILD_DIR)
	$(CC) -ffreestanding -c -g kernel/lib/printf.c -o $(BUILD_DIR)/printf.o

$(KERNEL_ELF): $(KERNEL_OBJ) $(DRIVER_OBJS) $(LIB_OBJS) linker.ld kernel/arch/x86_64/boot/_start.asm
	@echo "Assembling entry..."
	nasm -f elf64 kernel/arch/x86_64/boot/_start.asm -o $(BUILD_DIR)/start.o
	@echo "Linking kernel ELF..."
	ld -T linker.ld -o $(KERNEL_ELF) $(BUILD_DIR)/start.o $(KERNEL_OBJ) $(DRIVER_OBJS) $(LIB_OBJS)

run: iso
	@echo "Launching QEMU with ISO..."
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/orion.iso -serial stdio -no-reboot -no-shutdown

debug: $(KERNEL_ELF)
	@echo "Launching QEMU paused for GDB..."
	qemu-system-x86_64 -s -S -kernel $(KERNEL_ELF) -serial stdio

iso: $(KERNEL_ELF)
	@echo "Creating ISO with GRUB..."
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "Orion OS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '  boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD_DIR)/orion.iso $(ISO_DIR)

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(ISO_DIR)

lint:
	@echo "Running lint checks..."
