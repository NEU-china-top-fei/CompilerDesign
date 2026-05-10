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
#include "sysy.tab.hpp"
using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<Basenode> &ast);
extern int yydebug;
extern int yylex();
int reg_cnt = 0;
int cnt_if = 0, cur_offset = 0;
int globalcnt = 0;
std::vector<std::string> break_tag;
std::vector<std::string> continue_tag;
// bool if_terminate = false;

inline ST<int> *constTable;
inline ST<ele> *varTable;
inline ST<std::string> *funcTable;
std::map<std::string, std::string> name2op;

void process_lib()
{
  constTable = new ST<int>();
  varTable = new ST<ele>();
  funcTable = new ST<std::string>();
  funcTable->add("getint", "i32");
  funcTable->add("getarray", "i32");
  funcTable->add("putint", "");
  funcTable->add("putch", "");
  funcTable->add("putarray", "");
  funcTable->add("getch", "i32");
  funcTable->add("starttime", "");
  funcTable->add("stoptime", "");
}

void lexer_output(){
  int token;
  while ((token = yylex()) != 0) {
        cout << "Token类型: " << token << "\t";
        
        // 根据你 .l 文件中定义的 yylval 逻辑，打印具体的值
        switch (token) {
            case INT_CONST:
                cout << "<整数常量, " << yylval.int_val << ">" << endl;
                break;
            case IDENT:
                cout << "<标识符, " << *(yylval.str_val) << ">" << endl;
                delete yylval.str_val; // 注意：你在 .l 中 new 了一个 string，测试完毕后要手动释放，防止内存泄漏
                break;
            case IF:      cout << "<关键字, " << yylval.op << ">" << endl; break;
            case ELSE:    cout << "<关键字, " << yylval.op << ">" << endl; break;
            case LE:      cout << "<操作符, " << yylval.op << ">" << endl; break;
            case EQ:      cout << "<操作符, " << yylval.op << ">" << endl; break;
            case INT:     cout << "<关键字, int>" << endl; break;
            case RETURN:  cout << "<关键字, return>" << endl; break;
            default:
                // 如果是单字符（如 '+', '-', ';'），直接强转打印
                if (token < 256) {
                    cout << "<单字符, '" << (char)token << "'>" << endl;
                } else {
                    cout << "<其他Token>" << endl;
                }
                break;
        }
    }
}
int main(int argc, const char *argv[])
{
  process_lib();
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];
  std::ostringstream os;
  
  // yydebug = 1;
  yyin = fopen(input, "r");
  assert(yyin);
  string m=string(mode);
  if(m=="-lexer"){
    freopen(output, "w", stdout);
    lexer_output();
    std::cout << os.str();
    return 0;
  }
  
  unique_ptr<Basenode> ast;
  auto ret = yyparse(ast);
  assert(!ret);
  if(m=="-parser"){
    freopen(output, "w", stdout);
    std::cout << os.str();
    ast->dump(0);
    return 0;
  }
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
  os << "decl @getint() : i32 " << std::endl
     << "decl @getch() : i32" << std::endl
     << "decl @getarray(*i32) : i32" << std::endl
     << "decl @putint(i32)" << std::endl
     << "decl @putch(i32) " << std::endl
     << "decl @putarray(i32, *i32)" << std::endl
     << "decl @starttime()" << std::endl
     << "decl @stoptime()" << std::endl;
  if (m == "-koopa")
  {
    freopen(output, "w", stdout);
    std::cout << os.str();
    ast->dumpcode();
  }
  else if (m == "-riscv")
  {
    koopa_program_t program;
    ostringstream buffer;
    streambuf *obuffer = cout.rdbuf();
    cout.rdbuf(buffer.rdbuf());
    std::cout << os.str();
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
