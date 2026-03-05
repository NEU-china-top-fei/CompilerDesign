%code requires {
  #include <memory>
  #include <string>
  #include "AST.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<Basenode> &ast, const char *s);

using namespace std;

%}


%parse-param { std::unique_ptr<Basenode> &ast }


%union {
  std::string *str_val;
  int int_val;
  Basenode* ast_val; 
}


%token <int_val>INT RETURN INT_CONST
%token <str_val> IDENT
%type <ast_val> CompUnit FuncDef FuncType Block Stmt Number



%%


CompUnit
  : FuncDef {
    auto astroot=make_unique<CompUnit>();
    astroot->func_def = unique_ptr<Basenode>($1);
    ast=move(astroot);
  }
  ;


FuncDef
  : FuncType IDENT '(' ')' Block {
    auto thisfunc=new Funcdef();
    thisfunc->func_type = unique_ptr<Basenode>($1);
    thisfunc->ident = *unique_ptr<string>($2);
    thisfunc->block = unique_ptr<Basenode>($5);
    $$ = thisfunc;
  }
  ;


FuncType
  : INT {
    auto thisftype=new Functype();
    thisftype->tpname="int";
    $$ = thisftype;
  }
  ;

Block
  : '{' Stmt '}' {
    auto thisblock = new Block();
    thisblock->stmt=unique_ptr<Basenode>($2);
    $$ = thisblock;
  }
  ;

Stmt
  : RETURN Number ';' {
    auto thisstmt=new Stmt();
    thisstmt->number=unique_ptr<Basenode>($2);
    $$=thisstmt;
  }
  ;

Number
  : INT_CONST {
    auto thisnumber=new Number();
    thisnumber->num=to_string($1);
    $$ = thisnumber;
  }
  ;

%%

void yyerror(unique_ptr<Basenode> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
