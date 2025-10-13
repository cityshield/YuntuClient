#!/bin/bash

# 盛世云图客户端构建脚本

set -e

echo "========================================"
echo "    盛世云图客户端 - 构建脚本"
echo "========================================"

# 检查 Qt 环境
if [ -z "$Qt6_DIR" ]; then
    echo "错误: 未设置 Qt6_DIR 环境变量"
    echo "请设置 Qt6_DIR，例如："
    echo "  export Qt6_DIR=/path/to/Qt/6.5.3/clang_64"
    exit 1
fi

echo "Qt6 目录: $Qt6_DIR"

# 创建构建目录
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置 CMake
echo ""
echo "配置 CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DQt6_DIR="$Qt6_DIR/lib/cmake/Qt6"

# 编译
echo ""
echo "开始编译..."
cmake --build . --config Release

# 完成
echo ""
echo "========================================"
echo "    编译完成！"
echo "========================================"
echo "可执行文件位置: $BUILD_DIR/YuntuClient"
echo ""
echo "运行程序："
echo "  cd $BUILD_DIR"
echo "  ./YuntuClient"
