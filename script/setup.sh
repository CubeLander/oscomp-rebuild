#!/bin/bash
# apt-cache search riscv
sudo apt update
sudo apt install -y clang-format
sudo apt install -y software-properties-common
sudo apt install -y gcc-12-riscv64-linux-gnu 
sudo apt install -y g++-12-riscv64-linux-gnu
sudo apt install -y crossbuild-essential-riscv64 
sudo apt install -y binutils-riscv64-linux-gnu-dbg 
sudo apt install -y binutils-riscv64-linux-gnu 
sudo apt install -y cpp-12-riscv64-linux-gnu 
sudo apt install -y libgcc-12-dev-riscv64-cross 
sudo apt install -y linux-riscv-6.8-headers-6.8.0-57
sudo apt install -y gcc-riscv64-unknown-elf
sudo apt install -y cmake
sudo apt install -y gdb
sudo apt install -y qemu-system-x86
sudo apt install -y qemu-system-riscv64
sudo apt install -y gdb-multiarch
sudo apt install -y pkg-config