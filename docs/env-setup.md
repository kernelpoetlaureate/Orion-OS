# Environment Setup for Orion OS

This document provides instructions for setting up the development environment for Orion OS.

## Prerequisites
- Docker (recommended) or a Linux host with the following tools:
  - `gcc`, `binutils`, `nasm`
  - `grub-mkrescue`, `xorriso`
  - `qemu-system-x86`

## Quick Start with Docker
1. Build the Docker image:
   ```bash
   docker build -t orion-dev .
   ```
2. Run the container:
   ```bash
   docker run --rm -it -v $(pwd):/workspace orion-dev /bin/bash
   ```

## Manual Setup
1. Install the required packages:
   ```bash
   sudo apt-get update && sudo apt-get install -y \
       build-essential nasm gcc binutils grub-pc-bin xorriso qemu-system-x86
   ```
2. Clone the repository:
   ```bash
   git clone https://github.com/kernelpoetlaureate/Orion-OS.git
   ```
3. Navigate to the project directory:
   ```bash
   cd Orion-OS
   ```

## Building and Running
- Build the kernel:
  ```bash
  make build
  ```
- Run the kernel in QEMU:
  ```bash
  make run
  ```
- Debug the kernel:
  ```bash
  make debug
  ```

## Cleaning Up
- Remove build artifacts:
  ```bash
  make clean
  ```
