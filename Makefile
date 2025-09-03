# Makefile for Orion OS

# Toolchain
CC = gcc
AS = nasm
LD = ld

# Directories
BUILD_DIR = build
SRC_DIR = src
ISO_DIR = iso

# Targets
all: build

build:
	@echo "Building the kernel..."
	# Add build commands here

run: build
	@echo "Running the kernel in QEMU..."
	qemu-system-x86_64 -cdrom $(ISO_DIR)/orion.iso

debug: build
	@echo "Running QEMU in debug mode..."
	qemu-system-x86_64 -s -S -cdrom $(ISO_DIR)/orion.iso

iso: build
	@echo "Creating bootable ISO..."
	# Add ISO creation commands here

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(ISO_DIR)

lint:
	@echo "Running lint checks..."
	# Add linting commands here
