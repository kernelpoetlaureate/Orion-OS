# Minimal Ubuntu-based development environment for Orion OS
FROM ubuntu:20.04

# Set non-interactive mode for apt
ENV DEBIAN_FRONTEND=noninteractive

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    nasm \
    gcc \
    binutils \
    grub-pc-bin \
    xorriso \
    qemu-system-x86 \
    git \
    ccache \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]
