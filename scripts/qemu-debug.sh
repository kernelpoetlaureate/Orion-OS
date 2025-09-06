#!/bin/bash
# Script to run QEMU in debug mode with the Orion OS kernel

QEMU=qemu-system-x86_64
ISO=iso/orion.iso

if [ ! -f "$ISO" ]; then
  echo "ISO file not found: $ISO"
  exit 1
fi

$QEMU -s -S -cdrom $ISO
