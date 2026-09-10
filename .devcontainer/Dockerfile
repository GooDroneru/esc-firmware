FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    coreutils \
    git \
    ninja-build \
    python3 \
    pkg-config \
    libusb-1.0-0-dev \
    libftdi1-dev \
    libhidapi-dev \
    libjaylink0 \
    libgpiod2t64 \
    libpython3.12t64 \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    gdb-multiarch \
    && rm -rf /var/lib/apt/lists/*

# ---------------------------------------------------------------------------
# K1921VK035 (NIIET ARM Cortex-M4F)
# ---------------------------------------------------------------------------
# ARM GCC comes from apt (gcc-arm-none-eabi 14.2.rel1). A dedicated OpenOCD
# build is still needed because Ubuntu's openocd has no k1921vk035 target.

# OpenOCD for K1921VK035 (from DCVostok, has k1921vk035 flash/target support)
RUN wget -qO /tmp/openocd-k1921vk.tar.gz \
        "https://github.com/DCVostok/openocd-k1921vk/releases/download/v0.12.0-k1921vk/xpack-openocd-k1921vk-0.12.0-k1921vk-linux-x64.tar.gz" && \
    mkdir -p /tools/openocd-k1921vk && \
    tar -xzf /tmp/openocd-k1921vk.tar.gz --strip-components=1 -C /tools/openocd-k1921vk && \
    rm /tmp/openocd-k1921vk.tar.gz && \
    ln -s /tools/openocd-k1921vk/bin/openocd /usr/local/bin/openocd-k1921vk

# ---------------------------------------------------------------------------
# CH32V203 (WCH RISC-V)
# ---------------------------------------------------------------------------
# ONLY the official WCH riscv-none-embed-gcc 8.2.0 toolchain produces working
# CH32V203 firmware (generic RISC-V GCC miscompiles the WCH fast-interrupt
# prologues: the sine start runs, then commutation interrupts die). It is a
# WINDOWS toolchain, stored in the dependencies repo via LFS and run through
# Wine - exactly like (.github/workflows/build.yml, "RISC-V toolchain (WCH
# Windows .exe via Wine)"). The toolchain itself is bind-mounted from
# ../dependencies (LFS must be checked out on the host), Wine sits here.

# Wine (i386 for the 32-bit WCH toolchain), same as CI
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        wine wine32:i386 \
    && rm -rf /var/lib/apt/lists/*

# OpenOCD for WCH (CH32V203)
RUN wget -qO /tmp/openocd-wch.tar.gz \
        "https://dl.registry.platformio.org/download/community-ch32v/tool/tool-openocd-riscv-wch/2.1100.260228/tool-openocd-riscv-wch-linux_x86_64-2.1100.260228.tar.gz" && \
    mkdir -p /tools/openocd-wch && \
    tar -xzf /tmp/openocd-wch.tar.gz -C /tools/openocd-wch && \
    rm /tmp/openocd-wch.tar.gz && \
    ln -s /tools/openocd-wch/bin/openocd /usr/local/bin/openocd-wch
