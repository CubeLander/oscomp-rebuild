#!/bin/bash
# apt-cache search riscv
sudo apt update
sudo apt install software-properties-common
sudo apt install gcc-12-riscv64-linux-gnu 
sudo apt install g++-12-riscv64-linux-gnu
sudo apt install crossbuild-essential-riscv64 
sudo apt install binutils-riscv64-linux-gnu-dbg 
sudo apt install binutils-riscv64-linux-gnu 
sudo apt install cpp-12-riscv64-linux-gnu 
sudo apt install libgcc-12-dev-riscv64-cross 
sudo apt install linux-riscv-6.8-headers-6.8.0-57
sudo apt install gcc-riscv64-unknown-elf
sudo apt install cmake
sudo apt install gdb
sudo apt install qemu-system-x86
sudo apt install qemu-system-riscv64
sudo apt install gdb-multiarch