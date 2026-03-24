#pragma once
#include "koopa.h"
#include <iostream>
#include <assert.h>
#include <string.h>

// code generation strategy:stack machine
// result of a single expression is stored at t0 register if no special instruction
std::map<koopa_raw_value_t, int> offset;
bool has_call = false; // 记录当前函数里到底有没有 call
int sp = 0;
int labelid = 0;
int retpos = 0;
int max_arg_stack_size = 0;
// judge if need stack
int judge(const koopa_raw_value_t &v)
{
    switch (v->kind.tag)
    {
    case KOOPA_RVT_ALLOC:
    case KOOPA_RVT_LOAD:
    case KOOPA_RVT_BINARY:
    case KOOPA_RVT_CALL:
        return 4;
    default:
        return 0;
    }
}
std::map<koopa_raw_binary_op_t, std::string> binop;

void load(const koopa_raw_value_t &value, const std::string &reg)
{
    if (value->kind.tag == KOOPA_RVT_INTEGER)
    {
        std::cout << "    li " << reg << " ," << value->kind.data.integer.value << std::endl;
    }
    else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        std::cout << "    la " << reg << ", " << value->name + 1 << std::endl;
        std::cout << "    lw " << reg << ", 0(" << reg << ")" << std::endl;
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
// void visit(const koopa_raw_binary_t &bin);
void visit(const koopa_raw_branch_t &bran);
void visit(const koopa_raw_call_t &call, const koopa_raw_value_t &present);
void visit(const koopa_raw_block_arg_ref_t &bar);
void visit(const koopa_raw_jump_t &jump)
{
    std::cout << "    j " << (jump.target->name) + 1 << std::endl
              << std::endl;
    // visit(jump.target);
}

void visit(const koopa_raw_branch_t &bran)
{

    load(bran.cond, std::string("t0"));
    std::cout << "    bnez " << "t0, " << (bran.true_bb->name) + 1 << std::endl;
    std::cout << "    j " << (bran.false_bb->name) + 1 << std::endl;
    std::cout << std::endl;
}
void visit(const koopa_raw_call_t &call, const koopa_raw_value_t &present)
{
    for (int i = 0; i < call.args.len; i++)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);

        if (i < 8)
        {
            load(arg, "a" + std::to_string(i));
        }
        else
        {
            load(arg, "t0");
            std::cout << "    sw t0, " << (i - 8) * 4 << "(sp)" << std::endl;
        }
    }
    std::cout << "    call " << call.callee->name + 1 << std::endl;
    if (present->ty->tag == KOOPA_RTT_INT32)
    {
        std::cout << "    sw a0, " << offset[present] << "(sp)" << std::endl;
    }
}
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
    if (has_call)
    {
        std::cout << "    lw ra," << retpos << "(sp)" << std::endl;
    }

    if (sp > 0)
    {
        std::cout << "    addi sp,sp," << sp << std::endl;
    }
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

    // std::cout << "    .globl main" << std::endl;
    // 先输出全局变量
    for (size_t i = 0; i < program.values.len; i++)
    {
        auto v = reinterpret_cast<koopa_raw_value_t>(program.values.buffer[i]);
        if (v->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
        {
            auto g = v->kind.data.global_alloc;
            std::string name = v->name + 1;

            std::cout << "    .data\n";
            std::cout << "    .globl " << name << "\n";
            std::cout << name << ":\n";

            if (g.init->kind.tag == KOOPA_RVT_INTEGER)
            {
                std::cout << "    .word " << g.init->kind.data.integer.value << "\n";
            }
            else
            {
                std::cout << "    .zero 4\n";
            }
        }
    }
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
    if (function->bbs.len == 0)
        return;
    offset.clear();
    has_call = false;
    max_arg_stack_size = 0;
    std::string name = std::string(function->name);
    int length = name.size();
    int frame_size = 0, sentence = function->bbs.len;
    for (size_t i = 0; i < function->bbs.len; i++)
    {
        auto bb = reinterpret_cast<koopa_raw_basic_block_t>(function->bbs.buffer[i]);
        for (size_t j = 0; j < bb->insts.len; j++)
        {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
            if (inst->kind.tag == KOOPA_RVT_CALL)
            {
                has_call = true;
                int args_len = inst->kind.data.call.args.len;
                // RISC-V 规定：如果参数 > 8，多出的部分放在栈上
                max_arg_stack_size = std::max(max_arg_stack_size, std::max(0, (args_len - 8) * 4));
            }
        }
    }
    frame_size = max_arg_stack_size;
    for (int i = 0; i < sentence; i++)
    {
        auto bs = reinterpret_cast<koopa_raw_basic_block_t>(function->bbs.buffer[i]);
        auto item = bs->insts;
        int anothersize = item.len;

        for (int j = 0; j < anothersize; j++)
        {
            koopa_raw_value_t instruct = reinterpret_cast<koopa_raw_value_t>(item.buffer[j]);

            int sizeal = judge(instruct);
            if (sizeal != 0)
            {
                offset[instruct] = frame_size;
                frame_size += sizeal;
            }
        }
    }
    for (int i = 0; i < function->params.len; i++)
    {
        auto param = reinterpret_cast<koopa_raw_value_t>(function->params.buffer[i]);
        offset[param] = frame_size;
        frame_size += 4;
    }
    if (has_call)
    {
        retpos = frame_size;
        frame_size += 4;
    }
    frame_size = (frame_size + 15) / 16 * 16;
    std::cout << std::endl
              << "    .text" << std::endl;
    std::cout
        << "    .globl " << name.substr(1, length) << std::endl;
    std::cout << name.substr(1, length) << ":" << std::endl;
    sp = frame_size;
    std::cout << "    addi sp,sp,-" << frame_size << std::endl;
    if (has_call)
    {
        std::cout << "    sw ra," << retpos << "(sp)" << std::endl;
    }

    for (int i = 0; i < function->params.len; i++)
    {
        auto parm = reinterpret_cast<koopa_raw_value_t>(function->params.buffer[i]);
        if (i < 8)
        {
            std::cout << "    sw a" << i << "," << offset[parm] << "(sp)" << std::endl;
        }
        else
        {
            std::cout << "    lw t0," << (i - 8) * 4 + sp << "(sp)" << std::endl;
            std::cout << "    sw t0," << offset[parm] << "(sp)" << std::endl;
        }
    }

    visit(function->bbs);
}
void visit(const koopa_raw_basic_block_t &bblock)
{
    if (strcmp(bblock->name, "%entry"))
        std::cout << (bblock->name + 1) << ":" << std::endl;
    visit(bblock->insts);
}
// 增加第二个参数 ret，代表这条 load 指令自己
void visit(const koopa_raw_load_t &load_data, const koopa_raw_value_t &ret)
{
    if (load_data.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        // 如果是全局变量，先用 la 获取地址
        std::cout << "    la t0, " << load_data.src->name + 1 << std::endl;
        std::cout << "    lw t0, 0(t0)" << std::endl;
    }
    else
    {
        // 否则从栈上读取
        std::cout << "    lw t0, " << offset[load_data.src] << "(sp)" << std::endl;
    }
    // 最后统一存入当前指令分配的栈空间
    std::cout << "    sw t0, " << offset[ret] << "(sp)" << std::endl;
}
void visit(const koopa_raw_store_t &store)
{
    load(store.value, std::string("t0"));
    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        std::cout << "    la t1, " << store.dest->name + 1 << std::endl;
        std::cout << "    sw t0, 0(t1)" << std::endl;
    }
    else
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
    case KOOPA_RVT_BRANCH:
        visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
        visit(kind.data.call, value);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:

        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}