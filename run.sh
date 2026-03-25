#!/bin/bash

# 检查是否传入了参数
if [ "$#" -ne 1 ]; then
    echo "用法: $0 <源代码文件.c/.sysy>"
    echo "示例: $0 hello.c"
    exit 1
fi

INPUT_FILE="$1"

# 检查文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 找不到文件 '$INPUT_FILE'"
    exit 1
fi

# 检查环境变量 CDE_LIBRARY_PATH 是否已设置
if [ -z "$CDE_LIBRARY_PATH" ]; then
    echo "警告: 环境变量 CDE_LIBRARY_PATH 未设置，链接步骤可能会失败！"
fi

# 提取不带后缀的文件名 (例如: hello.c -> hello)
BASENAME=$(basename "$INPUT_FILE")
FILENAME="${BASENAME%.*}"

ASM_FILE="${FILENAME}.S"
OBJ_FILE="${FILENAME}.o"
EXE_FILE="${FILENAME}"

echo -e "\n[\033[34m1/4\033[0m] 正在编译 SysY -> RISC-V 汇编 ($ASM_FILE)..."
./build/compiler -riscv "$INPUT_FILE" -o "$ASM_FILE"
if [ $? -ne 0 ]; then
    echo -e "\033[31m编译失败！\033[0m"
    exit 1
fi

echo -e "[\033[34m2/4\033[0m] 正在使用 clang 汇编 ($OBJ_FILE)..."
clang "$ASM_FILE" -c -o "$OBJ_FILE" -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
if [ $? -ne 0 ]; then
    echo -e "\033[31m汇编失败！\033[0m"
    exit 1
fi

echo -e "[\033[34m3/4\033[0m] 正在使用 ld.lld 链接运行时库 ($EXE_FILE)..."
ld.lld "$OBJ_FILE" -L"$CDE_LIBRARY_PATH/riscv32" -lsysy -o "$EXE_FILE"
if [ $? -ne 0 ]; then
    echo -e "\033[31m链接失败！请检查 CDE_LIBRARY_PATH 路径。\033[0m"
    exit 1
fi

echo -e "[\033[34m4/4\033[0m] 正在使用 QEMU 运行程序...\n"
echo "------------------- 程序输出 -------------------"

# 运行程序并捕获退出码
qemu-riscv32-static "$EXE_FILE"
RET_CODE=$?

echo -e "\n------------------------------------------------"
echo -e "\033[32m程序执行完毕，退出码 (Return/Exit Code): $RET_CODE\033[0m\n"

# 可选：清理临时文件（取消注释以启用）
# rm -f "$ASM_FILE" "$OBJ_FILE"