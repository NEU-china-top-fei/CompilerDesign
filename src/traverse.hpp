#pragma once
#include "koopa.h"
#include <iostream>
#include <assert.h>
#include <string>
//declaration
void visit(const koopa_raw_program_t& program);
void visit(const koopa_raw_slice_t& slice);
void visit(const koopa_raw_function_t& function);
void visit(const koopa_raw_basic_block_t& bblock);
void visit(const koopa_raw_value_t& value);
void visit(const koopa_raw_return_t& ret);
void visit(const koopa_raw_integer_t& Int);


void visit(const koopa_raw_integer_t& Int){
    std::cout<<"    li a0, "<<Int.value<<std::endl;
}
void visit(const koopa_raw_return_t& ret){
    visit(ret.value);
    std::cout<<"    ret"<<std::endl;
}
void visit(const koopa_raw_program_t& program){
    std::cout<<"    .text"<<std::endl;
    std::cout<<"    .globl main"<<std::endl;
    visit(program.values);
    visit(program.funcs);

}
void visit(const koopa_raw_slice_t& slice){
    //std::cout<<slice.len<<slice.kind<<std::endl;
    for(size_t i=0;i<slice.len;i++){
        //std::cout<<"test01";
        auto item=slice.buffer[i];
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            visit(reinterpret_cast<koopa_raw_function_t>(item));
            //std::cout<<"function call"<<std::endl;
            break;
        
        case KOOPA_RSIK_BASIC_BLOCK:
            visit(reinterpret_cast<koopa_raw_basic_block_t>(item));
            //std::cout<<"basic block"<<std::endl;
            break;
        case KOOPA_RSIK_VALUE:
            visit(reinterpret_cast<koopa_raw_value_t>(item));
            //std::cout<<"value"<<std::endl;
            break;
        default:
            //std::cout<<"default"<<std::endl;
            assert(false);
            break;
        }
        //std::cout<<"loop"<<std::endl;
    }
}
void visit(const koopa_raw_function_t& function){
    std::string name=std::string(function->name);
    int length=name.size();
    std::cout<<name.substr(1,length)<<":"<<std::endl;
    visit(function->bbs);
}
void visit(const koopa_raw_basic_block_t& bblock){
    visit(bblock->insts);
}
void visit(const koopa_raw_value_t& value){
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
        // 访问 return 指令
        visit(kind.data.ret);
        break;
        case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        visit(kind.data.integer);
        break;
        default:
        // 其他类型暂时遇不到
        assert(false);
    }

}