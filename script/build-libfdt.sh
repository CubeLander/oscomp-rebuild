#!/bin/bash

# === 配置变量 ===
CROSS_PREFIX=riscv64-linux-gnu
# === 环境检查 ===
command -v ${CROSS_PREFIX}-gcc >/dev/null 2>&1 || {
  echo "找不到交叉编译器 ${CROSS_PREFIX}-gcc，请先安装交叉工具链。" >&2
  exit 1
}

cd "$(dirname "$0")"
echo "当前工作目录是：$(pwd)"
INSTALL_DIR=$(pwd)/../build
cd $(pwd)/../vendor/dtc
# === 清理旧构建 ===
make clean >/dev/null 2>&1

# === 编译 ===
echo "开始交叉编译 libfdt 到 RISC-V..."

make libfdt \
  CC=${CROSS_PREFIX}-gcc \
  CPPFLAGS="-I. -I./libfdt" \
  NO_PYTHON=1

# === 安装 ===
mkdir -p "$INSTALL_DIR/include"
mkdir -p "$INSTALL_DIR/lib"

cp -v libfdt/libfdt.a "$INSTALL_DIR/lib/"
cp -v libfdt/*.h "$INSTALL_DIR/include/"

echo "✅ 编译完成！已安装到：$INSTALL_DIR"
