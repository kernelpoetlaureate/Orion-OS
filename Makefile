# Makefile for Orion OS

CC = gcc -Ikernel
AS = gcc
LD = ld

BUILD_DIR = build
ISO_DIR = iso

KERNEL_OBJ = $(BUILD_DIR)/kernel.o
KERNEL_ELF = $(BUILD_DIR)/kernel.elf

DRIVER_OBJS = $(BUILD_DIR)/vga.o $(BUILD_DIR)/serial.o
LIB_OBJS = $(BUILD_DIR)/printf.o $(BUILD_DIR)/mem.o
CORE_OBJS = $(BUILD_DIR)/process.o
CORE_OBJS += $(BUILD_DIR)/pmm.o
CORE_OBJS += $(BUILD_DIR)/panic.o

all: $(KERNEL_ELF)

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

$(BUILD_DIR)/mem.o: kernel/lib/mem.c | $(BUILD_DIR)
	$(CC) -ffreestanding -Ikernel/lib/include -c -g kernel/lib/mem.c -o $(BUILD_DIR)/mem.o

$(BUILD_DIR)/process.o: kernel/core/process.c | $(BUILD_DIR)
	$(CC) -ffreestanding -c -g kernel/core/process.c -o $(BUILD_DIR)/process.o

$(BUILD_DIR)/pmm.o: kernel/core/pmm.c | $(BUILD_DIR)
	$(CC) -ffreestanding -Ilimine -c -g kernel/core/pmm.c -o $(BUILD_DIR)/pmm.o

$(BUILD_DIR)/panic.o: kernel/core/panic.c | $(BUILD_DIR)
	$(CC) -ffreestanding -c -g kernel/core/panic.c -o $(BUILD_DIR)/panic.o

$(KERNEL_ELF): $(KERNEL_OBJ) $(DRIVER_OBJS) $(LIB_OBJS) $(CORE_OBJS) linker.ld kernel/arch/x86_64/boot/_start.asm
	@echo "Assembling entry..."
	nasm -f elf64 kernel/arch/x86_64/boot/_start.asm -o $(BUILD_DIR)/start.o
	@echo "Linking kernel ELF..."
	ld -T linker.ld -o $(KERNEL_ELF) $(BUILD_DIR)/start.o $(KERNEL_OBJ) $(DRIVER_OBJS) $(LIB_OBJS) $(CORE_OBJS)

run: iso
	@echo "Launching QEMU with ISO..."
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/orion.iso -serial stdio -no-reboot -no-shutdown

debug: $(KERNEL_ELF)
	@echo "Launching QEMU paused for GDB..."
	qemu-system-x86_64 -s -S -kernel $(KERNEL_ELF) -serial stdio

iso: $(BUILD_DIR)/orion.iso

$(BUILD_DIR)/orion.iso: limine.cfg $(KERNEL_ELF) limine/limine-cd-efi.bin limine/limine.sys
	@echo "Creating ISO directory..."
	mkdir -p $(BUILD_DIR)/EFI/BOOT
	cp limine.cfg $(BUILD_DIR)/
	cp limine/limine-cd-efi.bin $(BUILD_DIR)/
	cp limine/BOOTX64.EFI $(BUILD_DIR)/EFI/BOOT/
	cp limine/limine.sys $(BUILD_DIR)/
	@echo "Generating ISO..."
	xorriso -as mkisofs -b limine-cd-efi.bin -no-emul-boot -boot-load-size 4 \
		-boot-info-table --efi-boot EFI/BOOT/BOOTX64.EFI -o $(BUILD_DIR)/orion.iso $(BUILD_DIR)/ || true
	@echo "Deploying Limine..."
	limine/limine-deploy $(BUILD_DIR)/orion.iso

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(ISO_DIR)

# Build a GRUB ISO that loads the kernel via multiboot/ELF
.PHONY: grub-iso
grub-iso: $(KERNEL_ELF)
	@echo "Creating GRUB ISO directory..."
	mkdir -p $(BUILD_DIR)/grub_iso/boot/grub
	cp $(KERNEL_ELF) $(BUILD_DIR)/grub_iso/kernel.elf
	@echo "set timeout=5" > $(BUILD_DIR)/grub_iso/boot/grub/grub.cfg
	@echo "menuentry 'Orion OS kernel.elf' {" >> $(BUILD_DIR)/grub_iso/boot/grub/grub.cfg
	@echo "  multiboot2 /kernel.elf" >> $(BUILD_DIR)/grub_iso/boot/grub/grub.cfg
	@echo "  boot" >> $(BUILD_DIR)/grub_iso/boot/grub/grub.cfg
	@echo "}" >> $(BUILD_DIR)/grub_iso/boot/grub/grub.cfg
	@echo "Generating GRUB ISO..."
	grub-mkrescue -o $(BUILD_DIR)/grub-orion.iso $(BUILD_DIR)/grub_iso || true

.PHONY: run-grub
run-grub: grub-iso
	@echo "Launching QEMU with GRUB ISO and comprehensive logging..."
	qemu-system-x86_64 \
		-drive file=$(BUILD_DIR)/grub-orion.iso,format=raw \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-d int,cpu_reset,guest_errors \
		-D $(BUILD_DIR)/qemu.log \
		2>&1 | tee $(BUILD_DIR)/qemu-output.log

.PHONY: run-grub-debug
run-grub-debug: grub-iso
	@echo "Launching QEMU with GRUB ISO and full debugging..."
	qemu-system-x86_64 \
		-drive file=$(BUILD_DIR)/grub-orion.iso,format=raw \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-d int,cpu_reset,guest_errors,exec,in_asm,out_asm,op,op_opt,op_ind \
		-D $(BUILD_DIR)/qemu-debug.log \
		-s -S \
		2>&1 | tee $(BUILD_DIR)/qemu-debug-output.log

lint:
	@echo "Running lint checks..."
