#!/bin/bash
set -ex

# 进入源码目录
cd "/root/workspace/oscomp-reb/vendor/musl"

# 确保输出目录存在
mkdir -p "/root/workspace/oscomp-reb/build/lib"

# 配置musl - 注意：我们仍然需要一个临时安装目录来获取头文件
TEMP_INSTALL_DIR="/root/workspace/oscomp-reb/build/vendor/musl/temp_install"
mkdir -p "$TEMP_INSTALL_DIR"

# 配置musl
echo "配置 musl..."
./configure \
  --prefix="$TEMP_INSTALL_DIR" \
  --target=riscv64-linux-gnu \
  --disable-shared \
	--disable-float \
  CFLAGS="-g3 -O0 -fno-omit-frame-pointer -mcmodel=medany -mabi=lp64d" \
#	LDFLAGS="-static -L/usr/lib/gcc-cross/riscv64-linux-gnu/13/ -lgcc" \
  CC=riscv64-linux-gnu-gcc

# 构建musl
echo "构建 musl..."
make clean
make -j32

# 直接复制库文件到目标位置而不是安装
echo "复制 musl 库到目标位置..."
cp lib/libc.a "/root/workspace/oscomp-reb/build/lib/libc.a"

# 安装头文件到临时目录
make install

echo "musl构建完成"
