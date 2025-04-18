#!/bin/bash

# 检查分支名称参数是否存在
if [ -z "$1" ]; then
    echo "错误：必须提供分支名称！"
    echo "用法：$0 <branch-name>"
    exit 1
fi

BRANCH_NAME=$1
COMMIT_MESSAGE="Initial commit of $BRANCH_NAME"

# 创建并切换到新分支
git checkout -b "$BRANCH_NAME"

# 添加所有变更（包括未跟踪文件）
git add .

# 提交变更
git commit -m "$COMMIT_MESSAGE"

# 输出结果
echo "已创建分支 '$BRANCH_NAME' 并提交初始变更。"