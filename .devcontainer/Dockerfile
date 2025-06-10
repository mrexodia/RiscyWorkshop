FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Base packages
RUN apt update \
 && apt upgrade -y \
 && apt install -y --no-install-recommends \
    software-properties-common \
    build-essential \
    curl \
    wget \
    lsb-release \
    gnupg \
    cmake \
    ninja-build \
    ripgrep \
    gdb \
    libc6-dev \
    p7zip-full \
    python3-venv \
    python-is-python3 \
    python3-pip \
    sudo \
    git \
 && rm -rf /var/lib/apt/lists/*

# LLVM 20 (https://apt.llvm.org)
RUN apt update \
 && wget https://apt.llvm.org/llvm.sh \
 && chmod +x llvm.sh \
 && ./llvm.sh 20 \
 && rm llvm.sh \
 && rm -rf /var/lib/apt/lists/*

# llvm-mingw (https://github.com/mstorsjo/llvm-mingw)
ARG LLVM_MINGW=llvm-mingw-20250528-ucrt-ubuntu-22.04-x86_64
RUN wget https://github.com/mstorsjo/llvm-mingw/releases/download/20250528/${LLVM_MINGW}.tar.xz \
 && tar -xf ${LLVM_MINGW}.tar.xz \
 && rm ${LLVM_MINGW}.tar.xz \
 && mv ${LLVM_MINGW} llvm-mingw

# NoVNC (https://github.com/theasp/docker-novnc)
ENV TZ=Europe/Warsaw
RUN apt update \
 && apt install -y --no-install-recommends \
    fluxbox \
    novnc \
    supervisor \
    x11vnc \
    xterm \
    xvfb \
 && rm -rf /var/lib/apt/lists/*
COPY novnc /novnc
ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8 \
    LC_ALL=C.UTF-8 \
    DISPLAY=:0.0 \
    DISPLAY_WIDTH=1024 \
    DISPLAY_HEIGHT=768 \
    RUN_XTERM=yes \
    RUN_FLUXBOX=yes
RUN cp /usr/share/novnc/vnc_lite.html /usr/share/novnc/index.html

# Wine
RUN dpkg --add-architecture i386 \
 && apt update \
 && apt install -y --no-install-recommends \
    wine \
    wine64 \
    wine32 \
 && rm -rf /var/lib/apt/lists/*

LABEL org.opencontainers.image.source=https://github.com/mrexodia/riscyworkshop
