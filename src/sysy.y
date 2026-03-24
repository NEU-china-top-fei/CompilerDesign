%code requires {
  #include <memory>
  #include <string>
  #include "AST.hpp"
  #include <vector>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"
#include <vector>

// 声明 lexer 函数和错误处理函数
int yylex();
extern int reg_cnt;
void yyerror(std::unique_ptr<Basenode> &ast, const char *s);

using namespace std;

%}
%define parse.trace

%parse-param { std::unique_ptr<Basenode> &ast }

%union {
  std::string *str_val;
  int int_val;
  Basenode* ast_val; 
  const char *op;
  std::vector<std::unique_ptr<Basenode>> *astlist;
}

%token INT RETURN CONST VOID
%token <int_val> INT_CONST
%token <str_val> IDENT
%type <ast_val> CompUnit Decl ConstDecl BType ConstDef ConstInitVal VarDecl VarDef InitVal BlockItem 
%type <ast_val> OptExp Optfuncfp Optfuncrp 
%type <ast_val> FuncDef Block Stmt Number Exp LVal PEXp UExp UOp MExp AExp RExp EExp LAExp LOExp ConstExp FuncFParam FuncFParams FuncRParams
%token LE GE EQ NE AND OR IF ELSE WHILE BREAK CONTINUE
%type <op> HelpAdd HelpE HelpR HelpM
%type <astlist> ConstDefList VarDefList BlockItemList Blockop FuncRlist FuncFlist ConstexpList InitList ConstinitList 
%type <astlist> Optconstexp

/* 解决 Dangling Else 的 shift/reduce 冲突 */
%nonassoc LOWER_THEN_ELSE
%nonassoc ELSE

%start Root

%%
Root
  : CompUnit {
    ast = unique_ptr<Basenode>($1);
  }

CompUnit
  : CompUnit Decl {
    auto astroot = new CompUnit();
    astroot->decl = unique_ptr<Basenode>($2);
    astroot->compunit = unique_ptr<Basenode>($1);
    astroot->which = 1;
    $$ = astroot;
  }
  | CompUnit FuncDef {
    auto astroot = new CompUnit();
    astroot->func_def = unique_ptr<Basenode>($2);
    astroot->compunit = unique_ptr<Basenode>($1);
    astroot->which = 2;
    $$ = astroot;
  }
  | Decl {
    auto astroot = new CompUnit();
    astroot->compunit = nullptr;
    astroot->decl = unique_ptr<Basenode>($1);
    astroot->which = 1;
    $$ = astroot;
  }
  | FuncDef {
    auto astroot = new CompUnit();
    astroot->compunit = nullptr;
    astroot->func_def = unique_ptr<Basenode>($1);
    astroot->which = 2;
    $$ = astroot;
  }
  ;

OptExp
  : /* empty */ { $$ = nullptr; }
  | Exp { $$ = $1; }
  ;

LVal
  : IDENT Optconstexp {
    auto thisl = new Lval();
    thisl->ident = *($1); 
    delete $1; // 修复内存泄露：清空 lexer 传来的 string 指针
    if($2) {
        thisl->exp = std::move(*$2);
        delete $2; // 修复内存泄露：清空 new 出来的 vector 指针
    }
    $$ = thisl;
  }
  ;

Decl
  : ConstDecl{
    auto thisd = new Decl();
    thisd->constde = unique_ptr<Basenode>($1);
    thisd->which = 1;
    $$ = thisd;
  }
  | VarDecl {
    auto thisd = new Decl();
    thisd->varde = unique_ptr<Basenode>($1);
    thisd->which = 2;
    $$ = thisd;
  }
  ;

InitList
  : InitVal {
    auto node = new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$ = node;
  }
  | InitList ',' InitVal {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  }
  ;

ConstexpList 
  : '[' ConstExp ']' {
    auto node = new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($2));
    $$ = node;
  }
  | ConstexpList '[' ConstExp ']' {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  }
  ;

Optconstexp
  : /* empty */ { $$ = nullptr; }
  | ConstexpList { $$ = $1; }
  ;

ConstinitList
  : ConstInitVal {
    auto node = new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$ = node;
  }
  | ConstinitList ',' ConstInitVal {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  }
  ;

ConstDefList
  : ConstDef{
    auto node = new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$ = node;
  }
  | ConstDefList ',' ConstDef{
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  }
  ;

VarDefList
  : VarDef {
    auto node = new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$ = node;
  }
  | VarDefList ',' VarDef {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  }
  ;

BlockItemList
  : BlockItem {
    auto node = new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$ = node;
  }
  | BlockItemList BlockItem {
    ($1)->push_back(unique_ptr<Basenode>($2));
    $$ = $1;
  }
  ;

Blockop 
  : /* empty */ { $$ = new std::vector<unique_ptr<Basenode>>; }
  | BlockItemList { $$ = $1; }
  ;

ConstDecl
  : CONST BType ConstDefList ';' {
    auto thisc = new ConstDecl();
    thisc->btype = unique_ptr<Basenode>($2);
    thisc->constdef = std::move(*$3);
    delete $3;
    $$ = thisc;
  }
  ;

BType
  : INT {
    auto thisb = new Btype();
    thisb->t = std::string("i32");
    $$ = thisb;
  }
  | VOID {
    auto thisb = new Btype();
    thisb->t = std::string("");
    $$ = thisb;
  }
  ;

ConstDef
  : IDENT Optconstexp '=' ConstInitVal {
    auto thisc = new Constdef();
    thisc->ident = *($1);
    delete $1;
    if ($2) {  // 致命错误修复：增加判空，防止标量导致解引用空指针奔溃
        thisc->constexp = std::move(*$2);
        delete $2;
    }
    thisc->constinit = unique_ptr<Basenode>($4);
    $$ = thisc;
  };

ConstInitVal
  : ConstExp {
    auto thisc = new Constinit();
    thisc->cexp = unique_ptr<Basenode>($1);
    thisc->which = 1;
    $$ = thisc;
  }
  | '{' '}' {
    auto l = new Constinit();
    l->which = 2;
    $$ = l;
  }
  | '{' ConstinitList '}' {
    auto l = new Constinit();
    l->constinit = std::move(*$2);
    delete $2;
    l->which = 2;
    $$ = l;
  }
  ;

VarDecl 
  : BType VarDefList ';' {
    auto thisv = new Vardecl();
    thisv->btype = unique_ptr<Basenode>($1);
    thisv->vardef = std::move(*$2);
    delete $2;
    $$ = thisv;
  };

VarDef
  : IDENT Optconstexp {
    auto thisv = new Vardef();
    thisv->ident = *($1);
    delete $1;
    if($2) {
      thisv->constexp = std::move(*$2);
      delete $2;
    }
    thisv->which = 1;
    $$ = thisv;
  }
  | IDENT Optconstexp '=' InitVal {
    auto thisv = new Vardef();
    thisv->ident = *($1);
    delete $1;
    if($2) {
      thisv->constexp = std::move(*$2);
      delete $2;
    }
    thisv->initval = unique_ptr<Basenode>($4);
    thisv->which = 2;
    $$ = thisv;
  };

InitVal
  : Exp {
    auto thisi = new Initval();
    thisi->exp = unique_ptr<Basenode>($1);
    thisi->which = 1;
    $$ = thisi;
  }
  | '{' '}' {
    auto l = new Initval();
    l->which = 2;
    $$ = l;
  }
  | '{' InitList '}' {
    auto l = new Initval();
    l->which = 2;
    l->initval = std::move(*$2);
    delete $2;
    $$ = l;
  }
  ;

Optfuncfp
  : /* empty */ { $$ = nullptr; }
  | FuncFParams { $$ = $1; }
  ;

Optfuncrp
  : /* empty */ { $$ = nullptr; }
  | FuncRParams { $$ = $1; }
  ;

FuncDef
  : BType IDENT '(' Optfuncfp ')' Block {
    auto thisfunc = new Funcdef();
    thisfunc->func_type = unique_ptr<Basenode>($1);
    thisfunc->ident = *($2);
    delete $2;
    thisfunc->funcfparams = unique_ptr<Basenode>($4);
    thisfunc->block = unique_ptr<Basenode>($6);
    $$ = thisfunc;
  }
  ;

FuncFlist 
  : FuncFParam {
    auto node = new std::vector<std::unique_ptr<Basenode>>();
    node->push_back(unique_ptr<Basenode>($1));
    $$ = node;
  }
  | FuncFlist ',' FuncFParam {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  }
  ;

FuncFParams
  : FuncFlist {
    auto f = new Funcfparams();
    f->funcfparams = std::move(*$1);
    delete $1;
    $$ = f;
  }
  ;

FuncFParam
  : BType IDENT {
    auto f = new Funcfparam();
    f->btype = unique_ptr<Basenode>($1);
    f->ident = *($2);
    delete $2;
    $$ = f;
  }
  | BType IDENT '[' ']' Optconstexp {
    auto f = new Funcfparam();
    f->btype = unique_ptr<Basenode>($1);
    f->ident = *($2);
    delete $2;
    if ($5) { // 致命错误修复：增加判空，并修复了遗漏 $$ = f; 的问题
        f->constexp = std::move(*$5);
        delete $5;
    }
    $$ = f; // 原代码这里漏了返回值
  }
  ;

Block
  : '{' Blockop '}' {
    auto thisb = new Block();
    thisb->blockitem = std::move(*$2);
    delete $2;
    $$ = thisb;
  }
  ;

BlockItem
  : Stmt {
    auto thisb = new BlockItem();
    thisb->stmt = unique_ptr<Basenode>($1);
    thisb->which = 2;
    $$ = thisb;
  }
  | Decl {
    auto thisb = new BlockItem();
    thisb->decl = unique_ptr<Basenode>($1);
    thisb->which = 1;
    $$ = thisb;
  }
  ;
  
Stmt
  : LVal '=' Exp ';'{
    auto thiss = new Stmt();
    thiss->lval = unique_ptr<Basenode>($1);
    thiss->exp = unique_ptr<Basenode>($3);
    thiss->which = 1;
    $$ = thiss;
  }
  | RETURN OptExp ';' {
    auto thisstmt = new Stmt();
    thisstmt->optexp = unique_ptr<Basenode>($2);
    thisstmt->which = 4;
    $$ = thisstmt;
  }
  | Block {
    auto thiss = new Stmt();
    thiss->block = unique_ptr<Basenode>($1);
    thiss->which = 3;
    $$ = thiss;
  }
  | IF '(' Exp ')' Stmt %prec LOWER_THEN_ELSE { // 修复 Shift/Reduce
    auto thiss = new Stmt();
    thiss->exp = unique_ptr<Basenode>($3);
    thiss->stmt = unique_ptr<Basenode>($5);
    thiss->optstmt = nullptr;
    thiss->which = 5;
    $$ = thiss;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt { // 修复 Shift/Reduce
    auto thiss = new Stmt();
    thiss->exp = unique_ptr<Basenode>($3);
    thiss->stmt = unique_ptr<Basenode>($5);
    thiss->optstmt = unique_ptr<Basenode>($7);
    thiss->which = 5;
    $$ = thiss;
  }
  | OptExp ';'  {
    auto thiss = new Stmt();
    thiss->optexp = unique_ptr<Basenode>($1);
    thiss->which = 2;
    $$ = thiss;
  }
  | WHILE '(' Exp ')' Stmt {
    auto thiss = new Stmt();
    thiss->which = 6;
    thiss->exp = unique_ptr<Basenode>($3);
    thiss->stmt = unique_ptr<Basenode>($5);
    thiss->optstmt = nullptr;
    $$ = thiss;
  }
  | BREAK ';' {
    auto thiss = new Stmt();
    thiss->which = 7;
    $$ = thiss;
  }
  | CONTINUE ';' {
    auto thiss = new Stmt();
    thiss->which = 8;
    $$ = thiss;
  }
  ;

Exp 
  : LOExp {
    auto thisexp = new Exp();
    thisexp->loexp = unique_ptr<Basenode>($1);
    $$ = thisexp;
  }
  ;

PEXp
  : '(' Exp ')' {
    auto thispexp = new PExp();
    thispexp->exp = unique_ptr<Basenode>($2);
    thispexp->which = 1;
    $$ = thispexp;
  }
  | LVal {
    auto thisp = new PExp();
    thisp->lval = unique_ptr<Basenode>($1);
    thisp->which = 2;
    $$ = thisp;
  }
  | Number  {
    auto thispexp = new PExp();
    thispexp->number = unique_ptr<Basenode>($1);
    thispexp->which = 3;
    $$ = thispexp;    
  }
  ;

UExp 
  : PEXp{
    auto thisuexp = new UExp();
    thisuexp->pexp = unique_ptr<Basenode>($1);
    thisuexp->which = 1;
    $$ = thisuexp;
  }
  | IDENT '(' Optfuncrp ')' {
    auto u = new UExp();
    u->ident = *($1);
    delete $1;
    u->funcrparams = unique_ptr<Basenode>($3);
    u->which = 2;
    $$ = u;
  }
  | UOp UExp{
    auto thisuexp = new UExp();
    thisuexp->uop = unique_ptr<Basenode>($1);
    thisuexp->uexp = unique_ptr<Basenode>($2);
    thisuexp->which = 3;
    $$ = thisuexp;
  }
  ;

UOp
  : '+' {
    auto thisuop = new UOp();
    thisuop->op = "+";
    $$ = thisuop;
  }
  | '-' {
    auto thisuop = new UOp();
    thisuop->op = "-";
    $$ = thisuop;
  }
  | '!' {
    auto thisuop = new UOp();
    thisuop->op = "!";
    $$ = thisuop;
  }
  ;

FuncRlist 
  : Exp {
    auto f = new std::vector<std::unique_ptr<Basenode>>();
    f->push_back(unique_ptr<Basenode>($1));
    $$ = f;
  }
  | FuncRlist ',' Exp {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$ = $1;
  };

FuncRParams 
  : FuncRlist {
    auto f = new Funcrparams();
    f->explist = std::move(*$1);
    delete $1;
    $$ = f;
  }
  ;

HelpM 
  : '*' { $$ = "*"; }
  | '/' { $$ = "/"; }
  | '%' { $$ = "%"; }
  ;

MExp 
  : UExp{
    auto thisme = new MExp();
    thisme->uexp = unique_ptr<Basenode>($1);
    thisme->which = 1;
    $$ = thisme;
  }
  | MExp HelpM UExp
  {
    auto thisme = new MExp();
    thisme->mexp = unique_ptr<Basenode>($1);
    thisme->op = $2;
    thisme->uexp = unique_ptr<Basenode>($3);
    thisme->which = 2;
    $$ = thisme;
  }
  ;

HelpAdd
  : '+' { $$ = "+"; }
  | '-' { $$ = "-"; }
  ;

AExp
  : MExp{
    auto thisa = new AExp();
    thisa->mexp = unique_ptr<Basenode>($1);
    thisa->which = 1;
    $$ = thisa;
  }
  | AExp HelpAdd MExp{
    auto thisa = new AExp();
    thisa->aexp = unique_ptr<Basenode>($1);
    thisa->op = $2;
    thisa->mexp = unique_ptr<Basenode>($3);
    thisa->which = 2;
    $$ = thisa;
  }
  ;

HelpR
  : LE  { $$ = "<="; }
  | GE  { $$ = ">="; }
  | '<' { $$ = "<"; }
  | '>' { $$ = ">"; }
  ;

RExp 
  : AExp  {
    auto thisr = new RExp();
    thisr->aexp = unique_ptr<Basenode>($1);
    thisr->which = 1;
    $$ = thisr;
  }
  | RExp HelpR AExp {
    auto thisr = new RExp();
    thisr->rexp = unique_ptr<Basenode>($1);
    thisr->op = $2;
    thisr->aexp = unique_ptr<Basenode>($3);
    thisr->which = 2;
    $$ = thisr;
  }
  ;

HelpE
  : EQ { $$ = "=="; }
  | NE { $$ = "!="; }
  ;

EExp 
 : RExp {
    auto thise = new EExp();
    thise->rexp = unique_ptr<Basenode>($1);
    thise->which = 1;
    $$ = thise;
 }
 | EExp HelpE RExp{
    auto thise = new EExp();
    thise->eexp = unique_ptr<Basenode>($1);
    thise->op = $2;
    thise->rexp = unique_ptr<Basenode>($3);
    thise->which = 2;
    $$ = thise; 
 }
 ;
 
LAExp
  : EExp 
  {
    auto thisl = new LAExp();
    thisl->eexp = unique_ptr<Basenode>($1);
    thisl->which = 1;
    $$ = thisl;  
  }
  | LAExp AND EExp
  {
    auto thisl = new LAExp();
    thisl->laexp = unique_ptr<Basenode>($1);
    thisl->eexp = unique_ptr<Basenode>($3);
    thisl->which = 2;
    $$ = thisl;
  }
  ;

LOExp
  : LAExp 
  {
    auto thisl = new LOExp();
    thisl->laexp = unique_ptr<Basenode>($1);
    thisl->which = 1;
    $$ = thisl;
  }
  | LOExp OR LAExp
  {
    auto thisl = new LOExp();
    thisl->loexp = unique_ptr<Basenode>($1);
    thisl->laexp = unique_ptr<Basenode>($3);
    thisl->which = 2;
    $$ = thisl;
  }
  ;

Number
  : INT_CONST {
    auto thisnumber = new Number();
    thisnumber->num = to_string($1);
    $$ = thisnumber;
  }
  ;

ConstExp 
  : Exp {
    auto thisc = new Constexp();
    thisc->exp = unique_ptr<Basenode>($1);
    $$ = thisc;
  }
%%

void yyerror(unique_ptr<Basenode> &ast, const char *s) {
  cerr << "error: " << s << endl;
}