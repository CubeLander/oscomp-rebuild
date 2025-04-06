#!/bin/bash
# 脚本名称: download_submodules.sh
# 功能: 下载并初始化项目中的所有子模块

set -e  # 出错时停止执行

echo "开始下载和初始化所有子模块..."

# 递归初始化和更新所有子模块
git submodule update --init --recursive

# 检查更新是否成功
if [ $? -eq 0 ]; then
    echo "子模块下载和初始化完成！"
else
    echo "子模块操作失败，请检查错误信息。"
    exit 1
fi

# 列出当前所有子模块状态
echo "当前子模块状态:"
git submodule status

echo "完成！"