#pragma once
#include "koopa.h"
#include <iostream>
#include <assert.h>
#include <string>

// code generation strategy:stack machine
// result of a single expression is stored at t0 register if no special instruction
std::map<koopa_raw_value_t, int> offset;
int sp = 0;
// judge if return
bool judge(const koopa_raw_value_t &v)
{
    bool isreturn = v->ty->tag != KOOPA_RTT_UNIT;
    bool isalloc = v->kind.tag == KOOPA_RVT_ALLOC;
    return isreturn || isalloc;
}
std::map<koopa_raw_binary_op_t, std::string> binop;

void load(const koopa_raw_value_t &value, const std::string &reg)
{
    if (value->kind.tag == KOOPA_RVT_INTEGER)
    {
        std::cout << "    li " << reg << " ," << value->kind.data.integer.value << std::endl;
    }
    else
    {

        std::cout << "    lw " << reg << " ," << offset[value] << "(sp)" << std::endl;
    }
}
// declaration
void visit(const koopa_raw_program_t &program);
void visit(const koopa_raw_slice_t &slice);
void visit(const koopa_raw_function_t &function);
void visit(const koopa_raw_basic_block_t &bblock);
void visit(const koopa_raw_value_t &value);
void visit(const koopa_raw_return_t &ret);
void visit(const koopa_raw_integer_t &Int);
void visit(const koopa_raw_binary_t &bin);

void visit(const koopa_raw_binary_t &bin, const koopa_raw_value_t &ret)
{
    load(bin.lhs, std::string("t1"));
    load(bin.rhs, std::string("t2"));
    std::cout << binop[bin.op] << std::endl;
    std::cout << "    sw t0, " << offset[ret] << "(sp)" << std::endl;
}
void visit(const koopa_raw_integer_t &Int)
{
}

void visit(const koopa_raw_return_t &ret)
{

    // visit(ret.value, std::string("a0"));
    if (ret.value)
    {
        load(ret.value, std::string("a0"));
    }
    std::cout << "    addi sp,sp," << sp << std::endl;
    std::cout << "    ret" << std::endl;
}
void visit(const koopa_raw_program_t &program)
{
    binop[KOOPA_RBO_NOT_EQ] = std::string("    sub t1,t1,t2\n    snez t0,t1");
    binop[KOOPA_RBO_EQ] = std::string("     sub t1,t1,t2\n    seqz t0,t1");
    binop[KOOPA_RBO_GT] = std::string("    sgt t0 ,t1,t2");
    binop[KOOPA_RBO_LT] = std::string("    slt t0 ,t1,t2");
    binop[KOOPA_RBO_GE] = std::string("    slt t0 ,t1,t2\n    xori t0,t0,1");
    binop[KOOPA_RBO_LE] = std::string("    sgt t0 ,t1,t2\n    xori t0,t0,1");
    binop[KOOPA_RBO_ADD] = std::string("    add t0 ,t1,t2");
    binop[KOOPA_RBO_SUB] = std::string("    sub t0 ,t1,t2");
    binop[KOOPA_RBO_MUL] = std::string("    mul t0 ,t1,t2");
    binop[KOOPA_RBO_DIV] = std::string("    div t0 ,t1,t2");
    binop[KOOPA_RBO_MOD] = std::string("    rem t0 ,t1,t2");
    binop[KOOPA_RBO_AND] = std::string("    and t0 ,t1,t2");
    binop[KOOPA_RBO_OR] = std::string("    or t0 ,t1,t2");
    binop[KOOPA_RBO_XOR] = std::string("    xor t0 ,t1,t2");
    binop[KOOPA_RBO_SHL] = std::string("    sll t0 ,t1,t2");
    binop[KOOPA_RBO_SHR] = std::string("    srl t0 ,t1,t2");
    binop[KOOPA_RBO_SAR] = std::string("    sra t0 ,t1,t2");

    std::cout << "    .text" << std::endl;
    // std::cout << "    .globl main" << std::endl;
    visit(program.values);
    visit(program.funcs);
}
void visit(const koopa_raw_slice_t &slice)
{
    // std::cout<<slice.len<<slice.kind<<std::endl;
    for (size_t i = 0; i < slice.len; i++)
    {
        // std::cout<<"test01";
        auto item = slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            visit(reinterpret_cast<koopa_raw_function_t>(item));
            // std::cout<<"function call"<<std::endl;
            break;

        case KOOPA_RSIK_BASIC_BLOCK:
            visit(reinterpret_cast<koopa_raw_basic_block_t>(item));
            // std::cout<<"basic block"<<std::endl;
            break;
        case KOOPA_RSIK_VALUE:
            visit(reinterpret_cast<koopa_raw_value_t>(item));
            // std::cout<<"value"<<std::endl;
            break;
        default:
            // std::cout<<"default"<<std::endl;
            assert(false);
            break;
        }
        // std::cout<<"loop"<<std::endl;
    }
}
void visit(const koopa_raw_function_t &function)
{
    offset.clear();
    std::string name = std::string(function->name);
    int length = name.size();
    int frame_size = 0, sentence = function->bbs.len;
    for (int i = 0; i < sentence; i++)
    {
        auto bs = reinterpret_cast<koopa_raw_basic_block_t>(function->bbs.buffer[i]);
        auto item = bs->insts;
        int anothersize = item.len;
        for (int j = 0; j < anothersize; j++)
        {
            koopa_raw_value_t instruct = reinterpret_cast<koopa_raw_value_t>(item.buffer[j]);
            if (judge(instruct))
            {
                offset[instruct] = frame_size;
                frame_size += 4;
            }
        }
    }
    frame_size = (frame_size + 23) / 16 * 16;
    sp = frame_size;
    std::cout
        << "    .globl " << name.substr(1, length) << std::endl;
    std::cout << name.substr(1, length) << ":" << std::endl;
    // 开辟函数栈
    std::cout << "    addi sp,sp,-" << frame_size << std::endl;
    visit(function->bbs);
}
void visit(const koopa_raw_basic_block_t &bblock)
{
    visit(bblock->insts);
}
// 增加第二个参数 ret，代表这条 load 指令自己
void visit(const koopa_raw_load_t &load, const koopa_raw_value_t &ret)
{
    // 1. 从目标变量(@x)中读出值放到 t0
    std::cout << "    lw t0, " << offset[load.src] << "(sp)" << std::endl;
    // 2. 把 t0 存回当前 load 指令(%1)专属的栈空间中！
    std::cout << "    sw t0, " << offset[ret] << "(sp)" << std::endl;
}
void visit(const koopa_raw_store_t &store)
{
    load(store.value, std::string("t0"));
    std::cout << "    sw t0" << " , " << offset[store.dest] << "(sp)" << std::endl;
}

void visit(const koopa_raw_value_t &value)
{
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        // 二元表达式
        visit(kind.data.binary, value);
        break;
    case KOOPA_RVT_ALLOC:
        break;

    case KOOPA_RVT_LOAD:
        visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        visit(kind.data.store);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}