#!/bin/bash
# 创建文件系统镜像脚本

# 确保使用bash执行
if [ -z "$BASH_VERSION" ]; then
    exec bash "$0" "$@"
    exit $?
fi

set -e

# 检查参数
if [ $# -lt 3 ]; then
    echo "用法: $0 <源目录> <输出镜像路径> <镜像大小MB> [强制重建]"
    exit 1
fi

SOURCE_DIR="$1"
OUTPUT_IMG="$2"
SIZE_MB="$3"
FORCE_REBUILD="${4:-0}"

# 创建输出目录（如果不存在）
mkdir -p "$(dirname "$OUTPUT_IMG")"

# 如果镜像已存在且不是强制重建，检查是否需要重建
if [ -f "$OUTPUT_IMG" ] && [ "$FORCE_REBUILD" != "1" ]; then
    # 获取源目录中最新文件的修改时间 - 使用更兼容的方式
    LATEST_SOURCE=$(find "$SOURCE_DIR" -type f -exec stat -c '%Y' {} \; | sort -n | tail -1)
    
    # 获取镜像文件的修改时间
    IMG_TIME=$(stat -c %Y "$OUTPUT_IMG")
    
    # 如果镜像比源文件新，则跳过重建 - 使用简单比较
    if [ "$IMG_TIME" -gt "$LATEST_SOURCE" ]; then
        echo "镜像文件 $OUTPUT_IMG 已经是最新的，跳过重建"
        exit 0
    fi
fi

echo "创建 $SIZE_MB MB 大小的 ext4 文件系统镜像..."

# 创建空白镜像文件
dd if=/dev/zero of="$OUTPUT_IMG" bs=1M count="$SIZE_MB"

# 创建 ext4 文件系统
mkfs.ext4 -F "$OUTPUT_IMG"

# 创建挂载点 - 使用更安全的方式
MOUNT_POINT="/tmp/fs_mount_$$"
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