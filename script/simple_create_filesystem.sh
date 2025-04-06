#!/bin/bash
# 创建文件系统镜像脚本 - 简化版本

set -e

# 检查参数
if [ $# -lt 3 ]; then
    echo "用法: $0 <源目录> <输出镜像路径> <镜像大小MB>"
    exit 1
fi

SOURCE_DIR="$1"
OUTPUT_IMG="$2"
SIZE_MB="$3"

# 创建输出目录
mkdir -p "$(dirname "$OUTPUT_IMG")"

echo "创建 $SIZE_MB MB 大小的 ext4 文件系统镜像..."

# 创建空白镜像文件
dd if=/dev/zero of="$OUTPUT_IMG" bs=1M count="$SIZE_MB"

# 创建 ext4 文件系统
mkfs.ext4 -F "$OUTPUT_IMG"

# 创建挂载点
MOUNT_POINT="/tmp/fs_mount_temp"
mkdir -p "$MOUNT_POINT"

# 挂载文件系统
echo "挂载镜像..."
sudo mount -o loop "$OUTPUT_IMG" "$MOUNT_POINT"

# 复制文件
echo "复制文件到镜像..."
sudo cp -a "$SOURCE_DIR"/* "$MOUNT_POINT"/

# 设置权限
echo "设置权限..."
sudo chown -R root:root "$MOUNT_POINT"
sudo chmod 755 "$MOUNT_POINT"

# 卸载文件系统
echo "卸载镜像..."
sudo umount "$MOUNT_POINT"

# 清理挂载点
rmdir "$MOUNT_POINT"

echo "文件系统镜像创建完成: $OUTPUT_IMG"