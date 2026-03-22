#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"
#include "koopa.h"
#include "traverse.hpp"
#include <sstream>
#include "ST.hpp"
using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<Basenode> &ast);
int reg_cnt = 0;
int cnt_if = 0;
int globalcnt = 0;
std::vector<std::string> break_tag;
std::vector<std::string> continue_tag;
// bool if_terminate = false;
std::map<std::string, std::string> name2op;
/*
getint(): i32
getch(): i32
getarray(*i32): i32
putint(i32)
putch(i32)
putarray(i32, *i32)
starttime()
stoptime()
*/
void process_lib(ST<string> *t)
{
  t->add("getint", "i32");
  t->add("getarray", "i32");
  t->add("putint", "");
  t->add("putch", "");
  t->add("putarray", "");
  t->add("getch", "i32");
  t->add("starttime", "");
  t->add("stoptime", "");
}
int main(int argc, const char *argv[])
{

  constTable = new ST<int>();
  varTable = new ST<ele>();
  funcTable = new ST<string>();
  globalconst = new ST<int>();
  globalvar = new ST<string>();
  process_lib(funcTable);
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  yyin = fopen(input, "r");
  assert(yyin);
  unique_ptr<Basenode> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  if (string(mode) == "-koopa")
  {
    freopen(output, "w", stdout);
    std::cout << "decl @getint() : i32 " << std::endl
              << "decl @getch() : i32" << std::endl
              << "decl @getarray(*i32) : i32" << std::endl
              << "decl @putint(i32)" << std::endl
              << "decl @putch(i32) " << std::endl
              << "decl @putarray(i32, *i32)" << std::endl
              << "decl @starttime()" << std::endl
              << "decl @stoptime()" << std::endl;
    name2op["+"] = "add";
    name2op["-"] = "sub";
    name2op["*"] = "mul";
    name2op["/"] = "div";
    name2op["%"] = "mod";
    name2op["<"] = "lt";
    name2op[">"] = "gt";
    name2op["<="] = "le";
    name2op[">="] = "ge";
    name2op["=="] = "eq";
    name2op["!="] = "ne";
    name2op["&&"] = "and";
    name2op["||"] = "or";
    name2op["!"] = "eq";
    // std::cerr << "before ast" << std::endl;
    ast->dumpcode();
  }
  if (string(mode) == "-riscv")
  {
    koopa_program_t program;
    ostringstream buffer;
    streambuf *obuffer = cout.rdbuf();
    cout.rdbuf(buffer.rdbuf());
    ast->dumpcode();
    koopa_error_code_t ret_code = koopa_parse_from_string(buffer.str().c_str(), &program);
    assert(ret_code == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    cout.rdbuf(obuffer);
    freopen(output, "w", stdout);
    koopa_delete_program(program);
    visit(raw);
    koopa_delete_raw_program_builder(builder);
  }

  return 0;
}
