#!/bin/bash

# Install development packages (TODO: --no-install-recommends)
apt update && apt upgrade -y
apt install -y --no-install-recommends git build-essential curl wget lsb-release software-properties-common gnupg cmake ninja-build ripgrep gdb p7zip-full python3-venv python-is-python3 python3-pip sudo

# Install Wine
dpkg --add-architecture i386 && apt-get update && apt-get install --no-install-recommends wine wine32

# Install llvm
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 20

# Install llvm-mingw (TODO: install in /llvm-mingw)
wget https://github.com/mstorsjo/llvm-mingw/releases/download/20250528/llvm-mingw-20250528-ucrt-ubuntu-22.04-x86_64.tar.xz
xz -d llvm-mingw-20250528-ucrt-ubuntu-22.04-x86_64.tar.xz
tar -xf llvm-mingw-20250528-ucrt-ubuntu-22.04-x86_64.tar
rm llvm-mingw-20250528-ucrt-ubuntu-22.04-x86_64.tar
