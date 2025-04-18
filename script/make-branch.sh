#!/bin/bash

# 提示用户输入分支名称
read -p "请输入要创建的分支名称: " BRANCH_NAME

# 检查分支名称是否为空
if [ -z "$BRANCH_NAME" ]; then
    echo "错误：分支名称不能为空！"
    exit 1
fi

# 检查分支是否已存在
if git show-ref --verify --quiet refs/heads/"$BRANCH_NAME"; then
    echo "错误：分支 '$BRANCH_NAME' 已经存在！"
    exit 1
fi

COMMIT_MESSAGE="Initial commit of $BRANCH_NAME"

# 创建并切换到新分支
git checkout -b "$BRANCH_NAME"

# 添加所有变更（包括未跟踪文件）
git add .

# 提交变更
git commit -m "$COMMIT_MESSAGE"

# 输出结果
echo "已创建分支 '$BRANCH_NAME' 并提交初始变更。"
