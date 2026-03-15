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
std::map<std::string, std::string> name2op;
int main(int argc, const char *argv[])
{

  constTable = new ST<int>();
  varTable = new ST<string>();
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
