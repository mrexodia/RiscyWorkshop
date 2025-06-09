FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Warsaw

RUN dpkg --add-architecture i386 \
 && apt-get update \
 && apt upgrade -y \
 && apt-get install -y --no-install-recommends \
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
    wine \
    wine32 \
 && wget https://apt.llvm.org/llvm.sh \
 && chmod +x llvm.sh \
 && ./llvm.sh 20 \
 && rm llvm.sh \
 && rm -rf /var/lib/apt/lists/*

ARG LLVM_MINGW=llvm-mingw-20250528-ucrt-ubuntu-22.04-x86_64
RUN wget https://github.com/mstorsjo/llvm-mingw/releases/download/20250528/${LLVM_MINGW}.tar.xz \
 && tar -xf ${LLVM_MINGW}.tar.xz \
 && rm ${LLVM_MINGW}.tar.xz \
 && mv ${LLVM_MINGW} llvm-mingw

LABEL org.opencontainers.image.source=https://github.com/mrexodia/riscyworkshop
