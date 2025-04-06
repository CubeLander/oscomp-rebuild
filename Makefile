MUSL_BUILD_DIR = $(shell pwd)/build-musl
MUSL_LIB = $(MUSL_BUILD_DIR)/lib/libc.a
MUSL_SCRIPT = script/build-musl.sh
MUSL_SOURCE = vendor/musl

# 目录设置
PROJECT_ROOT := $(shell pwd)
BUILD_DIR := $(PROJECT_ROOT)/build
KERNEL_DIR := $(PROJECT_ROOT)/kernel
USER_DIR := $(PROJECT_ROOT)/user
VENDOR_DIR := $(PROJECT_ROOT)/vendor
SCRIPT_DIR := $(PROJECT_ROOT)/script

# MUSL 构建目标
.PHONY: musl musl-clean build kernel busybox run gdb gdbc clean rebuild lwext4 example-with-musl cmake-build submodule
.PHONY: debug-all asm debug-symbols debug-elfinfo debug-headers debug-sections debug-strings debug-reloc debug-dynamic debug-size
.PHONY: fs-image fs-dirs fs-configs fs-install-busybox fs-install-user

submodule:
	@echo "更新子模块..."
	git submodule init
	git submodule update --recursive

# CMake构建
cmake-build:
	@echo "使用CMake构建项目..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# 快速构建内核，不包括文件系统镜像
kernel:
	@echo "构建内核..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make quick_build

# 完整构建
build:
	@echo "完整构建项目..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

# Busybox 构建
busybox:
	@echo "构建 Busybox..."
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make busybox_build

# 运行 QEMU
run: 
	qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -m 2G \
  -bios default \
  -kernel build/bin/riscv-pke \
  -drive file=build/disk_img/rootfs.img,format=raw,id=hd0 \
  -device virtio-blk-device,drive=hd0

# GDB 调试服务器
gdb:
	qemu-system-riscv64 \
  -machine virt \
  -m 2G \
  -nographic \
  -bios default \
  -kernel build/bin/riscv-pke \
  -drive file=build/disk_img/rootfs.img,format=raw,id=hd0 \
  -device virtio-blk-device,drive=hd0 \
  -s -S

# GDB 客户端
gdbc:
	riscv64-unknown-elf-gdb -x gdbinit.txt build/bin/riscv-pke -q

# 清理构建
clean:
	rm -rf build

# 重新构建
rebuild:
	rm -rf build
	@bash script/build.sh 
	@bash script/make_busybox.sh

# 构建 lwext4
lwext4:
	@bash script/build-lwext4.sh

# MUSL 相关构建
musl: $(MUSL_LIB)
$(MUSL_LIB): $(MUSL_SCRIPT) $(MUSL_SOURCE)
	@echo "构建 MUSL riscv64 静态库..."
	@chmod +x $(MUSL_SCRIPT)
	@$(MUSL_SCRIPT)
	@echo "MUSL 构建完成"

musl-clean:
	@echo "清理 MUSL 构建..."
	@rm -rf build-musl
	@echo "MUSL 清理完成"

# 使用MUSL的示例目标
example-with-musl: $(MUSL_LIB)
	@echo "使用MUSL静态库编译示例程序..."
	riscv64-unknown-elf-gcc -static -o example example.c \
		-L$(MUSL_BUILD_DIR)/lib \
		-I$(MUSL_SOURCE)/include \
		-lc

# ============ 新增目标 - 调试信息 ============

# 生成所有调试信息
debug-all:
	@echo "生成所有调试信息..."
	cd $(BUILD_DIR) && make kernel_debug_info

# 生成反汇编
asm:
	@echo "生成反汇编..."
	cd $(BUILD_DIR) && make kernel_disasm

# 生成符号表
debug-symbols:
	@echo "生成符号表..."
	cd $(BUILD_DIR) && make riscv-pke_symbols

# 生成ELF信息
debug-elfinfo:
	@echo "生成ELF信息..."
	cd $(BUILD_DIR) && make riscv-pke_elfinfo

# 生成头信息
debug-headers:
	@echo "生成头信息..."
	cd $(BUILD_DIR) && make riscv-pke_headers

# 生成段信息
debug-sections:
	@echo "生成段信息..."
	cd $(BUILD_DIR) && make riscv-pke_sections

# 生成字符串信息
debug-strings:
	@echo "生成字符串信息..."
	cd $(BUILD_DIR) && make riscv-pke_strings

# 生成重定位信息
debug-reloc:
	@echo "生成重定位信息..."
	cd $(BUILD_DIR) && make riscv-pke_relocations

# 生成动态信息
debug-dynamic:
	@echo "生成动态信息..."
	cd $(BUILD_DIR) && make riscv-pke_dynamic

# 生成大小信息
debug-size:
	@echo "生成大小信息..."
	cd $(BUILD_DIR) && make riscv-pke_size

# ============ 新增目标 - 文件系统镜像 ============

# 创建完整文件系统镜像
fs-image:
	@echo "创建文件系统镜像..."
	cd $(BUILD_DIR) && make create_system_image

# 创建文件系统目录结构
fs-dirs:
	@echo "创建文件系统目录结构..."
	cd $(BUILD_DIR) && make create_fs_dirs

# # 创建基本配置文件
# fs-configs:
# 	@echo "创建基本配置文件..."
# 	cd $(BUILD_DIR) && make create_fs_configs

# 安装 Busybox 到文件系统
fs-install-busybox:
	@echo "安装 Busybox 到文件系统..."
	cd $(BUILD_DIR) && make install_busybox_fs

# 安装用户程序到文件系统
fs-install-user:
	@echo "安装用户程序到文件系统..."
	cd $(BUILD_DIR) && make install_user_programs_fs

# 挂载文件系统镜像（仅用于调试）
fs-mount:
	@echo "挂载文件系统镜像..."
	sudo mkdir -p /mnt/rootfs
	sudo mount -o loop $(BUILD_DIR)/disk_img/rootfs.img /mnt/rootfs

# 卸载文件系统镜像
fs-unmount:
	@echo "卸载文件系统镜像..."
	sudo umount /mnt/rootfs