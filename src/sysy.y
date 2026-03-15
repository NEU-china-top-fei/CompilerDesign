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


%parse-param { std::unique_ptr<Basenode> &ast }

%union {
  std::string *str_val;
  int int_val;
  Basenode* ast_val; 
  const char *op;
  std::vector<std::unique_ptr<Basenode>> *astlist;
}

%token INT RETURN CONST
%token <int_val> INT_CONST
%token <str_val> IDENT
%type <ast_val> CompUnit Decl ConstDecl BType ConstDef ConstInitVal VarDecl VarDef InitVal BlockItem
%type <ast_val> FuncDef FuncType Block  Stmt Number Exp LVal PEXp UExp UOp MExp AExp RExp EExp LAExp LOExp ConstExp
%token LE GE EQ NE AND OR 
%type <op> HelpAdd HelpE HelpR HelpM
%type <astlist> ConstDefList VarDefList BlockItemList Blockop


%%


CompUnit
  : FuncDef {
    auto astroot=make_unique<CompUnit>();
    astroot->func_def = unique_ptr<Basenode>($1);
    ast=move(astroot);
  }
  ;
LVal
  : IDENT {
    auto thisl=new Lval();
    thisl->ident=*($1);
    $$=thisl;
  }
  ;
Decl
  : ConstDecl{
    auto thisd=new Decl();
    thisd->constde=unique_ptr<Basenode>($1);
    thisd->which=1;
    $$=thisd;
  }
  | VarDecl {
    auto thisd=new Decl();
    thisd->varde=unique_ptr<Basenode>($1);
    thisd->which=2;
    $$=thisd;
  }
  ;
ConstDefList
  : ConstDef{
    auto node=new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$=node;
  }
  | ConstDefList ',' ConstDef{
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$=$1;
  }
  ;
VarDefList
  : VarDef {
    auto node=new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$=node;
  }
  | VarDefList ',' VarDef {
    ($1)->push_back(unique_ptr<Basenode>($3));
    $$=$1;
  };
BlockItemList
  : BlockItem {
    auto node=new std::vector<std::unique_ptr<Basenode>>;
    node->push_back(unique_ptr<Basenode>($1));
    $$=node;
  }
  | BlockItemList BlockItem {
    ($1)->push_back(unique_ptr<Basenode>($2));
    $$=$1;
  }
Blockop 
  : {$$=new std::vector<unique_ptr<Basenode>>;}
  | BlockItemList {
    $$=$1;
  }
//"const" BType ConstDef {"," ConstDef} ";";
ConstDecl
  : CONST BType ConstDefList ';' {
    auto thisc=new ConstDecl();
    thisc->btype=unique_ptr<Basenode>($2);
    thisc->constdef=std::move(*($3));
    $$=thisc;
  }
  ;
BType
  : INT {
    auto thisb=new Btype();
    thisb->t=std::string("int");
    $$=thisb;
  };
// ConstDef :: = IDENT "=" ConstInitVal;
ConstDef
  : IDENT '=' ConstInitVal{
    auto thisc=new Constdef();
    thisc->ident=*($1);
    thisc->constinit=unique_ptr<Basenode>($3);
    $$=thisc;
  };
// ConstInitVal :: = ConstExp;
ConstInitVal
  : ConstExp {
    auto thisc=new Constinit();
    thisc->cexp=unique_ptr<Basenode>($1);
    $$=thisc;
  };
// VarDecl :: = BType VarDef { "," VarDef }";";
VarDecl 
  : BType VarDefList ';' {
    auto thisv=new Vardecl();
    thisv->btype=unique_ptr<Basenode>($1);
    thisv->vardef=std::move(*($2));
    $$=thisv;
  };
// VarDef :: = IDENT | IDENT "=" InitVal;
VarDef
  : IDENT {
    auto thisv=new Vardef();
    thisv->ident=*($1);
    thisv->which=1;
    $$=thisv;
  }
  | IDENT '=' InitVal {
    auto thisv=new Vardef();
    thisv->ident=*($1);
    thisv->initval=unique_ptr<Basenode>($3);
    thisv->which=2;
    $$=thisv;
  };
// InitVal :: = Exp;
InitVal
  : Exp {
    auto thisi=new Initval();
    thisi->exp=unique_ptr<Basenode>($1);
    $$=thisi;
  };
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
  : '{' Blockop '}' {
    //setreg();
    auto thisb=new Block();
    thisb->blockitem=std::move(*($2));
    $$ = thisb;
  }

  ;
BlockItem
  : Decl {
    auto thisb=new BlockItem();
    thisb->decl=unique_ptr<Basenode>($1);
    thisb->which=1;
    $$=thisb;
  }
  | Stmt {
    auto thisb=new BlockItem();
    thisb->stmt=unique_ptr<Basenode>($1);
    thisb->which=2;
    $$=thisb;
  };
// Stmt          ::= LVal "=" Exp ";"
//                 | "return" Exp ";";
Stmt
  : LVal '=' Exp ';'{
    auto thiss=new Stmt();
    thiss->lval=unique_ptr<Basenode>($1);
    thiss->exp=unique_ptr<Basenode>($3);
    thiss->which=1;
    $$=thiss;
  }
  | RETURN Exp ';' {
    auto thisstmt=new Stmt();
    thisstmt->exp=unique_ptr<Basenode>($2);
    thisstmt->which=2;
    $$=thisstmt;
  }
  ;

Exp 
  : LOExp {
    auto thisexp=new Exp();
    thisexp->loexp=unique_ptr<Basenode>($1);
    $$=thisexp;
  }
  ;

PEXp
  : '(' Exp ')' {
    auto thispexp=new PExp();
    thispexp->exp=unique_ptr<Basenode>($2);
    thispexp->which=1;
    $$=thispexp;
  }
  | LVal {
    auto thisp=new PExp();
    thisp->lval=unique_ptr<Basenode>($1);
    thisp->which=2;
    $$=thisp;
  }
  | Number  {
    auto thispexp=new PExp();
    thispexp->number=unique_ptr<Basenode>($1);
    thispexp->which=3;
    $$=thispexp;    
  }
  ;
UExp 
  : PEXp{
    auto thisuexp=new UExp();
    thisuexp->pexp=unique_ptr<Basenode>($1);
    thisuexp->which=1;
    $$=thisuexp;
  }
  | UOp UExp{
    auto thisuexp=new UExp();
    thisuexp->uop=unique_ptr<Basenode>($1);
    thisuexp->uexp=unique_ptr<Basenode>($2);
    thisuexp->which=2;
    $$=thisuexp;
  }
  ;
UOp
  : '+' {
    auto thisuop=new UOp();
    thisuop->op="+";
    //thisuop->which=1;
    $$=thisuop;
  }
  | '-' {
    auto thisuop=new UOp();
    thisuop->op="-";
    //thisuop->which=2;
    $$=thisuop;
  }
  | '!' {
    auto thisuop=new UOp();
    thisuop->op="!";
    //thisuop->which=3;
    $$=thisuop;
  }
  ;
HelpM 
  :'*'
  {
    $$="*";
  }
  |'/'
  {
    $$="/";
  }
  |'%' 
  {
    $$="%";
  }
  ;
MExp 
  : UExp{
    auto thisme=new MExp();
    thisme->uexp=unique_ptr<Basenode>($1);
    thisme->which=1;
    $$=thisme;
  }
  | MExp HelpM UExp
  {
    auto thisme=new MExp();
    thisme->mexp=unique_ptr<Basenode>($1);
    thisme->op=$2;
    thisme->uexp=unique_ptr<Basenode>($3);
    thisme->which=2;
    $$=thisme;
  }
  ;
HelpAdd
  : '+' {
    $$="+";
  }
  |'-'  {
    $$="-";
  }
  ;
AExp
  : MExp{
    auto thisa=new AExp();
    thisa->mexp=unique_ptr<Basenode>($1);
    thisa->which=1;
    $$=thisa;
  }
  | AExp HelpAdd MExp{
    auto thisa=new AExp();
    thisa->aexp=unique_ptr<Basenode>($1);
    thisa->op=$2;
    thisa->mexp=unique_ptr<Basenode>($3);
    thisa->which=2;
    $$=thisa;
  }
  ;
HelpR
  : LE{
    $$="<=";
  }
  |GE{
    $$=">=";
  }
  |'<'{
    $$="<";
  }
  |'>' {
    $$=">";
  }
  ;
RExp 
  : AExp  {
    auto thisr=new RExp();
    thisr->aexp=unique_ptr<Basenode>($1);
    thisr->which=1;
    $$=thisr;
  }
  | RExp HelpR AExp {
    auto thisr=new RExp();
    thisr->rexp=unique_ptr<Basenode>($1);;
    thisr->op=$2;
    thisr->aexp=unique_ptr<Basenode>($3);
    thisr->which=2;
    $$=thisr;
  }
  ;
HelpE
  : EQ{
    $$="==";
  }
  |NE{
    $$="!=";
  }
  ;
EExp 
 : RExp {
    auto thise=new EExp();
    thise->rexp=unique_ptr<Basenode>($1);
    thise->which=1;
    $$=thise;
 }
 | EExp HelpE RExp{
    auto thise=new EExp();
    thise->eexp=unique_ptr<Basenode>($1);
    thise->op=$2;
    thise->rexp=unique_ptr<Basenode>($3);
    thise->which=2;
    $$=thise; 
 }
 ;
 
LAExp
  : EExp 
  {
    auto thisl=new LAExp();
    thisl->eexp=unique_ptr<Basenode>($1);
    thisl->which=1;
    $$=thisl;  
  }
  | LAExp AND EExp
  {
    auto thisl=new LAExp();
    thisl->laexp=unique_ptr<Basenode>($1);
    thisl->eexp=unique_ptr<Basenode>($3);
    thisl->which=2;
    $$=thisl;
  }
  ;

LOExp
  : LAExp 
  {
    auto thisl=new LOExp();
    thisl->laexp=unique_ptr<Basenode>($1);
    thisl->which=1;
    $$=thisl;
  }
  | LOExp OR LAExp
  {
    auto thisl=new LOExp();
    thisl->loexp=unique_ptr<Basenode>($1);
    thisl->laexp=unique_ptr<Basenode>($3);
    thisl->which=2;
    $$=thisl;
  }
  ;
Number
  : INT_CONST {
    auto thisnumber=new Number();
    thisnumber->num=to_string($1);
    $$ = thisnumber;
  }
  ;
ConstExp 
  : Exp {
    auto thisc=new Constexp();
    thisc->exp=unique_ptr<Basenode>($1);
    $$=thisc;
  }
%%

void yyerror(unique_ptr<Basenode> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
