%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>


#include "sysy.tab.hpp"
int layer;
using namespace std;

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*

/*多行注释*/
%x MULTICO
/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%
\/\*    {
    BEGIN(MULTICO);
    layer=1;
}
<MULTICO>\*\/  {
    layer--;
    if(layer==0)BEGIN(INITIAL);
}
<MULTICO>\/\*  {
    layer++;
}
<MULTICO>.  {/*忽略*/}

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }

"int"           { return INT; }
"return"        { return RETURN; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { return yytext[0]; }

%%
