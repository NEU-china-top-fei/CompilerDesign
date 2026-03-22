#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <cxxabi.h>
#include "ST.hpp"
#include <vector>
#include <type_traits> // 包含 std::is_same_v 和 std::decay_t
extern int reg_cnt;
// extern bool if_terminate;
#define setreg() (reg_cnt = 0)
#define increg() (reg_cnt++)
#define getreg() (reg_cnt) // 返回当前语句的目标寄存器
#define decreg(x) (reg_cnt -= x)
extern std::vector<std::string> break_tag;
extern std::vector<std::string> continue_tag;
extern std::map<std::string, std::string> name2op;
extern int cnt_if;
inline std::string demangle(const char *name)
{
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> res{
        abi::__cxa_demangle(name, nullptr, nullptr, &status),
        std::free};
    return (status == 0) ? res.get() : name;
}
class Basenode
{
public:
    virtual ~Basenode() = default;
    virtual void dump() {

    };
    virtual std::string dumpcode()
    { // 返回立即数或者寄存器值或者变量的ident
        return std::string("");
    };
    virtual std::string retop()
    {
        return std::string("");
    };
    virtual std::string dumpcode(std::string &ident, bool isglobal) { return std::string(""); }
    virtual std::string dumpcode(Basenode *btype, bool isglobal) { return std::string(""); }
    virtual std::string dumpcode(bool judge) { return std::string(""); } // true for global when comes to value
    virtual int cal() { return 0; }
    virtual std::string addr() { return std::string(""); }
    virtual bool bdumpcode() { return false; } // return if terminated or has return value
};
inline std::string help_tri_short(Basenode *n1, Basenode *n2, const std::string &opname);

inline std::string *process_variable(std::string &temp)
{
    if (temp.empty())
        return nullptr;
    char prefix = temp.c_str()[0];
    bool isdigit = prefix >= '0' && prefix <= '9';
    std::string *ret = nullptr;
    if (prefix == '%')
        return nullptr;
    if (isdigit)
        return nullptr;
    auto c = constTable->find(temp);
    if (c)
    {
        temp = std::to_string(*c);
        return nullptr;
    }

    auto v = varTable->find(temp);
    if (v == nullptr)
        return nullptr;

    if (v->is_ptr)
        return &(v->val);

    return nullptr;
}
// int result = 1;
// if (lhs == 0) {
//   result = rhs != 0;
// }
// or

// int result=0;
//  if(lhs==1){result= rhs!=0}
//

inline std::string help_tri(Basenode *n1, Basenode *n2, const std::string &opname)
{
    std::string str1 = n1->dumpcode();
    std::string str2 = n2->dumpcode();
    std::string op = name2op[opname];

    // if (op == "and" || op == "or")
    // {
    //     int first = getreg();
    //     increg();
    //     int second = getreg();
    //     increg();
    //     std::cout << "    %" << first << " = ne " << str1 << " ,0" << std::endl;
    //     std::cout << "    %" << second << " = ne " << str2 << " ,0" << std::endl;
    //     str1 = "%" + std::to_string(first);
    //     str2 = "%" + std::to_string(second);
    // }

    std::string temp = "%" + std::to_string(getreg());
    increg();

    std::cout << "    " << temp << " = " << op << " " << str1 << " , " << str2 << std::endl;
    return temp;
}
// CompUnit      ::= [CompUnit] (Decl | FuncDef);
class CompUnit : public Basenode
{
public:
    std::unique_ptr<Basenode> compunit;
    std::unique_ptr<Basenode> decl;
    std::unique_ptr<Basenode> func_def;
    int which;
    void dump()
    {
        std::cout << "Compuit { ";
        func_def->dump();
        std::cout << " } ";
    }
    std::string dumpcode()
    {

        if (compunit != nullptr)
        {
            // std::cerr << "type called " << demangle(typeid(*this).name()) << " if comp " << std::endl;
            compunit->dumpcode();
        }
        if (which == 1)
            decl->dumpcode(true);
        else
        {
            // std::cerr << "good" << std::endl;
            func_def->dumpcode();
        }
        return "";
    }
};

// Decl          ::= ConstDecl | VarDecl;
class Decl : public Basenode
{
public:
    std::unique_ptr<Basenode> constde;
    std::unique_ptr<Basenode> varde;
    int which;
    std::string dumpcode(bool isglobal)
    {
        switch (which)
        {
        case 1:
            constde->dumpcode(isglobal);
            break;

        case 2:
            varde->dumpcode(isglobal);
            break;
        }
        return "";
    }
};
// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDecl : public Basenode
{
public:
    std::unique_ptr<Basenode> btype;
    std::vector<std::unique_ptr<Basenode>> constdef;
    std::string dumpcode(bool isglobal)
    {
        for (auto i = constdef.begin(); i != constdef.end(); i++)
        {
            (*i)->dumpcode(isglobal);
        }
        return "";
    }
};
// BType         ::= "int"| "void";
class Btype : public Basenode
{
public:
    std::string t;
    std::string dumpcode()
    {
        // if (t == "int")
        // {
        //     std::cout << "i32";
        // }
        // else if (t == "void")
        // {
        // }
        return t;
    }
};
// ConstDef :: = IDENT "=" ConstInitVal;
class Constdef : public Basenode
{
public:
    std::string ident;
    std::unique_ptr<Basenode> constinit;
    std::string dumpcode(bool isglobal)
    {

        constinit->dumpcode(ident, isglobal);
        return "";
    }
};
// ConstInitVal :: = ConstExp;
class Constinit : public Basenode
{
public:
    std::unique_ptr<Basenode> cexp;
    std::string dumpcode(std::string &ident, bool isglobal)
    {
        int result = cexp->cal();
        if (isglobal)
            globalconst->add(ident, result);
        else
            constTable->add(ident, result);
        return std::to_string(result);
    }
};
// VarDecl :: = BType VarDef { "," VarDef }";";
class Vardecl : public Basenode
{
public:
    std::unique_ptr<Basenode> btype;
    std::vector<std::unique_ptr<Basenode>> vardef;
    std::string dumpcode(bool isglobal)
    {
        for (auto i = vardef.begin(); i != vardef.end(); i++)
        {
            (*i)->dumpcode(btype.get(), isglobal);
        }
        return "";
    }
};
// VarDef :: = IDENT | IDENT "=" InitVal;
class Vardef : public Basenode
{
public:
    std::string ident;
    std::unique_ptr<Basenode> initval;
    int which;
    std::string dumpcode(Basenode *btype, bool isglobal)
    {
        std::string dis_tag = std::to_string(varTable->getcnt());
        std::string temp = "";
        if (which == 2)
            temp = initval->dumpcode();
        if (isglobal)
        {
            std::cout << "    global %" << ident << dis_tag << "  = alloc " << btype->dumpcode();

            if (which == 2)
            {
                std::cout << " , " << temp << std::endl;
            }
            else
            {
                std::cout << " , zeroinit" << std::endl;
            }
            globalvar->add(ident, ident + dis_tag);
        }
        else
        {
            std::cout << "    %" << ident << dis_tag << "  = alloc " << btype->dumpcode() << std::endl;
            ele new_e;
            new_e.is_ptr = true;
            new_e.val = ident + dis_tag;
            varTable->add(ident, new_e);
            if (which == 2)
                std::cout << "    store " << temp << ", %" << new_e.val << std::endl;
        }

        return "";
    }
};
// InitVal :: = Exp;
class Initval : public Basenode
{
public:
    std::unique_ptr<Basenode> exp;
    std::string dumpcode()
    {
        return exp->dumpcode();
    }
    int cal()
    {
        return exp->cal();
    }
};
//  FuncDef       ::= btype IDENT "(" [FuncFParams] ")" Block;
class Funcdef : public Basenode
{
public:
    std::unique_ptr<Basenode> func_type;
    std::string ident;
    std::unique_ptr<Basenode> funcfparams;
    std::unique_ptr<Basenode> block;
    void dump()
    {
        std::cout << " Funcdef { ";
        func_type->dump();
        std::cout << ident;
        block->dump();
        std::cout << " } ";
    }
    std::string dumpcode()
    {

        std::cout << "fun @" << ident << "(";
        varTable = varTable->enter_scope();
        if (funcfparams != nullptr)
            funcfparams->dumpcode();
        std::cout << ")";
        std::string temp = func_type->dumpcode();
        if (temp != "")
            std::cout << ": " << temp;
        funcTable->add(ident, temp);
        std::cout << " {" << std::endl;
        std::cout << "%entry:" << std::endl;
        bool ret = false;
        if (block != nullptr)
            ret = block->bdumpcode();
        if (!ret)
        {
            std::cout << "    ret ";
            if (func_type->dumpcode() == "i32")
                std::cout << "0";
        }

        std::cout << std::endl
                  << "}" << std::endl;
        varTable = varTable->parent;
        return "";
    }
};

// FuncFParams :: = FuncFParam{"," FuncFParam};
class Funcfparams : public Basenode
{
public:
    std::vector<std::unique_ptr<Basenode>> funcfparams;
    std::string dumpcode()
    {
        for (auto i = funcfparams.begin(); i != funcfparams.end(); i++)
        {
            std::string temp = (*i)->dumpcode();
            ele newe;
            newe.is_ptr = false;
            newe.val = temp;
            varTable->add(temp, newe);
            if ((*i) != funcfparams.back())
            {
                std::cout << ", ";
            }
        }
        return "";
    }
};
// FuncFParam :: = BType IDENT;
class Funcfparam : public Basenode
{
public:
    std::unique_ptr<Basenode> btype;
    std::string ident;
    std::string dumpcode()
    {
        std::cout << "%" << ident;
        std::string temp = btype->dumpcode();
        if (temp != "")
            std::cout << " : " << temp;
        return ident;
    }
};
//  Block         ::= "{" {BlockItem} "}";
//  BlockItem     ::= Decl | Stmt;
class Block : public Basenode
{
public:
    std::vector<std::unique_ptr<Basenode>> blockitem;
    bool bdumpcode()
    {
        constTable = constTable->enter_scope();
        varTable = varTable->enter_scope();
        bool isret = false;
        for (auto i = blockitem.begin(); i != blockitem.end(); i++)
        {
            // //std::cerr << "Processing BlockItem!" << std::endl; // 埋点
            isret = isret || (*i)->bdumpcode();
            if (isret)
                break;
        }
        constTable = constTable->parent;
        varTable = varTable->parent;
        return isret;
    }
};
class BlockItem : public Basenode
{
public:
    std::unique_ptr<Basenode> decl;
    std::unique_ptr<Basenode> stmt;
    int which;
    bool bdumpcode()
    {
        // //std::cerr << "Processing BlockItem! which = " << which << std::endl; // 重点打印 which
        switch (which)
        {
        case 1:
            decl->dumpcode();
            break;
        case 2:
        {
            bool as = stmt->bdumpcode();
            return as;
            break;
        }
        }
        return false;
    }
};
// Stmt          ::= LVal "=" Exp ";"
//                 | [Exp] ";"
//                 | Block
//                 | "return" [Exp] ";";
//                 | "if" "(" Exp ")" Stmt ["else" Stmt]
//                 | "while" "(" Exp ")" Stmt
//                 | "break" ";"
//                 | "continue" ";"
//[]    refers to repeat zero or one time
class Stmt : public Basenode
{
public:
    std::unique_ptr<Basenode> lval;
    std::unique_ptr<Basenode> exp;
    std::unique_ptr<Basenode> block;
    std::unique_ptr<Basenode> optexp;
    std::unique_ptr<Basenode> stmt;
    std::unique_ptr<Basenode> optstmt;

    void process_if_while(bool isloop = false) // return the header or tail flag
    {
        int cur = cnt_if++;
        std::string loopi = "%while_entry" + std::to_string(cur), theni = "%then" + std::to_string(cur), elsei = "%else" + std::to_string(cur), endi = "%end" + std::to_string(cur);
        if (isloop)
        {
            std::cout << "    jump " << loopi << std::endl
                      << std::endl;
            std::cout << loopi << ":" << std::endl;
            break_tag.push_back(endi);
            continue_tag.push_back(loopi);
        }
        std::string predic = this->exp->dumpcode();
        std::string *judge = process_variable(predic);

        std::string target = this->optstmt == nullptr ? endi : elsei; // 若没有else直接跳转到end
        if (judge)
        {
            std::cout << "    %" << getreg() << " = load %" << (*judge) << std::endl;
            std::cout << "    br %" << getreg() << "," << theni << "," << target << std::endl;
        }
        else
        {
            std::cout << "    br " << predic << "," << theni << "," << target << std::endl;
        }
        increg();
        std::cout << theni << ":" << std::endl;
        bool istern = this->stmt->bdumpcode();
        if (!istern)
        {
            if (!isloop)
                std::cout << "    jump " << endi << std::endl;
            else
                std::cout << "    jump " << loopi << std::endl;
        }

        if (this->optstmt != nullptr)
        {
            std::cout << elsei << ":" << std::endl;
            bool istern = this->optstmt->bdumpcode();
            if (!istern)
                std::cout << "    jump " << endi << std::endl;
        }
        std::cout << endi << ":" << std::endl;
        if (isloop)
        {
            break_tag.pop_back();
            continue_tag.pop_back();
        }
    }

    int which;
    bool bdumpcode()
    {
        switch (which)
        {
        case 1:
        {
            std::string value = exp->dumpcode();
            std::string ptr = lval->dumpcode(true);
            std::cout << "    store " << value << ", " << ptr << std::endl;
            return false;
            break;
        }
        case 2:
            if (optexp != nullptr)
                optexp->dumpcode();
            return false;
            break;
        case 3:
            return block->bdumpcode();
            break;
        case 4:
        {
            std::string temp = "0";
            if (optexp != nullptr)
                temp = optexp->dumpcode();
            std::cout << "    ret " << temp << std::endl;
            return true;
            break;
        }
        case 5:
        {
            process_if_while();
            return false;
        }
        break;
        case 6:
        {
            process_if_while(true);
            return false;
        }
        break;
        case 7:
        {
            if (!break_tag.empty())
            {
                std::cout << "    jump " << break_tag.back() << std::endl;
                return true;
            }
            return false;
        }
        break;
        case 8:
        {
            if (!continue_tag.empty())
            {
                std::cout << "    jump " << continue_tag.back() << std::endl;
                return true;
            }
            return false;
        }
        break;
        }
    }
};

// Number      ::= INT_CONST;
class Number : public Basenode
{
public:
    std::string num;
    void dump()
    {
        std::cout << num;
    }
    std::string dumpcode()
    {
        return num;
    }
    int cal()
    {
        return std::stoi(num);
    }
};

// Exp         ::= LOrExp;
class Exp : public Basenode
{
public:
    std::unique_ptr<Basenode> loexp;
    std::string dumpcode()
    {
        return loexp->dumpcode();
    }
    int cal()
    {
        return loexp->cal();
    }
};

class Lval : public Basenode
{
public:
    std::string ident;
    std::string addr()
    {
        return std::string("%") + (varTable->find(ident))->val;
    }
    std::string dumpcode(bool is_addr)
    {
        if (is_addr)
        {
            return std::string("%") + (varTable->find(ident))->val;
        }

        auto c = constTable->find(ident);
        if (c != nullptr)
            return std::to_string(*c);

        auto v = varTable->find(ident);
        if (v != nullptr)
        {
            if (v->is_ptr)
            {
                std::string reg = "%" + std::to_string(getreg());
                std::cout << "    " << reg << " = load %" << v->val << std::endl;
                increg();
                return reg;
            }
            else
                return "%" + v->val;
        }
        return "0";
    }
};
// PrimaryExp    ::= "(" Exp ")" | LVal | Number;
class PExp : public Basenode
{
public:
    std::unique_ptr<Basenode> exp;
    std::unique_ptr<Basenode> number;
    std::unique_ptr<Basenode> lval;
    int which; // indicate which production we will chose
    std::string dumpcode()
    {

        switch (which)
        {
        case 1:

            return exp->dumpcode();
            break;
        case 2:

            return lval->dumpcode(false);
        case 3:

            return number->dumpcode();
            break;
        default:
            break;
        }
        return "";
    }
    int cal()
    {
        switch (which)
        {
        case 1:
            return exp->cal();
            break;
        case 2:
            return lval->cal();
        case 3:
            return number->cal();
            break;
        }
        return 0;
    }
};
// UnaryExp :: = PrimaryExp | IDENT "("[FuncRParams] ")" | UnaryOp UnaryExp;
class UExp : public Basenode
{
public:
    std::unique_ptr<Basenode> pexp;
    std::unique_ptr<Basenode> uop;
    std::string ident;
    std::unique_ptr<Basenode> funcrparams;
    std::unique_ptr<Basenode> uexp;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return pexp->dumpcode();
            break;
        case 2:
        {
            std::string *temp = funcTable->find(ident);
            std::string reg = "";
            std::string args = "";
            if (funcrparams != nullptr)
                args = funcrparams->dumpcode();
            if (*temp != "")
            {
                reg = "%" + std::to_string(getreg());
                increg();
                std::cout << "    " << reg << " =  call @" << ident << "(" << args << ")" << std::endl;
            }
            else
                std::cout << "    call @" << ident << "(" << args << ")" << std::endl;
            increg();

            return reg;
        }
        break;

        case 3:
        {
            std::string temp = uexp->dumpcode();

            if (uop->retop() == "+")
                return temp;

            if (uop->retop() == "-")
            {
                std::string reg = "%" + std::to_string(getreg());
                std::cout << "    " << reg << " = sub 0, " << temp << std::endl;
                increg();
                return reg;
            }

            if (uop->retop() == "!")
            {
                std::string reg = "%" + std::to_string(getreg());
                std::cout << "    " << reg << " = eq " << temp << ", 0" << std::endl;
                increg();
                return reg;
            }
        }

        break;
        }
        return "";
    }
    int cal()
    {

        switch (which)
        {
        case 1:
            return pexp->cal();
            break;

        case 2:
            int temp = uexp->cal();
            if (uop->retop() == "-")
            {
                return -1 * temp;
            }
            else if (uop->retop() == "!")
            {
                return temp == 0;
            }
            else if (uop->retop() == "+")
            {
                return temp;
            }
            break;
        }
        return 0;
    }
};
// UnaryOp     ::= "+" | "-" | "!";
class UOp : public Basenode
{
public:
    std::string op;
    int which;
    std::string retop()
    {
        return op;
    }
};
// FuncRParams   ::= Exp {"," Exp};
class Funcrparams : public Basenode
{
public:
    std::vector<std::unique_ptr<Basenode>> explist;
    std::string dumpcode() // return the list
    {
        std::ostringstream args;
        for (auto i = explist.begin(); i != explist.end(); i++)
        {
            args << (*i)->dumpcode();
            if ((*i) != explist.back())
            {
                args << ",";
            }
        }
        return args.str();
    }
};
//  MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class MExp : public Basenode
{
public:
    std::unique_ptr<Basenode> uexp;
    std::string op;
    std::unique_ptr<Basenode> mexp;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return uexp->dumpcode();
            break;
        case 2:
            return help_tri(mexp.get(), uexp.get(), op);
            break;
        }
        return "";
    }
    int cal()
    {
        int temp2 = uexp->cal();

        switch (which)
        {
        case 1:
            return temp2;
            break;

        case 2:
            int temp1 = mexp->cal();
            if (op == "*")
            {
                return temp1 * temp2;
            }
            else if (op == "/")
            {
                return temp1 / temp2;
            }
            else
            {
                return temp1 % temp2;
            }
            break;
        }
        return 0;
    }
};

// AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
class AExp : public Basenode
{
public:
    std::unique_ptr<Basenode> mexp;
    std::string op;
    std::unique_ptr<Basenode> aexp;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return mexp->dumpcode();
            break;
        case 2:
            return help_tri(aexp.get(), mexp.get(), op);
            break;
        }
        return "";
    }
    int cal()
    {
        int temp2 = mexp->cal();

        switch (which)
        {
        case 1:
            return temp2;
            break;

        case 2:
            int temp1 = aexp->cal();
            if (op == "+")
            {
                return temp1 + temp2;
            }
            else if (op == "-")
            {
                return temp1 - temp2;
            }

            break;
        }
        return 0;
    }
};
// RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
class RExp : public Basenode
{
public:
    std::unique_ptr<Basenode> aexp;
    std::string op;
    std::unique_ptr<Basenode> rexp;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return aexp->dumpcode();
            break;
        case 2:
            return help_tri(rexp.get(), aexp.get(), op);

            break;
        }
        return "";
    }
    int cal()
    {
        int temp2 = aexp->cal();

        switch (which)
        {
        case 1:
            return temp2;
            break;

        case 2:
            int temp1 = rexp->cal();
            if (op == "<")
            {
                return temp1 < temp2;
            }
            else if (op == ">")
            {
                return temp1 > temp2;
            }
            else if (op == "<=")
            {
                return temp1 <= temp2;
            }
            else if (op == ">=")
            {
                return temp1 >= temp2;
            }
            break;
        }
        return 0;
    }
};
// EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
class EExp : public Basenode
{
public:
    std::unique_ptr<Basenode> rexp;
    std::unique_ptr<Basenode> eexp;
    std::string op;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            /* code */
            return rexp->dumpcode();
            break;

        case 2:
            return help_tri(eexp.get(), rexp.get(), op);
            break;
        }
        return "";
    }
    int cal()
    {
        int temp2 = rexp->cal();

        switch (which)
        {
        case 1:
            return temp2;
            break;

        case 2:
            int temp1 = eexp->cal();
            if (op == "==")
            {
                return temp1 == temp2;
            }
            else if (op == "!=")
            {
                return temp1 != temp2;
            }

            break;
        }
        return 0;
    }
};
// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
inline void help_pro()
{
}
class LAExp : public Basenode
{
public:
    std::unique_ptr<Basenode> eexp;
    std::unique_ptr<Basenode> laexp;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return eexp->dumpcode();
            break;
        case 2:
            return help_tri_short(laexp.get(), eexp.get(), std::string("&&"));
            break;
        }
        return "";
    }
    int cal()
    {
        int temp2 = eexp->cal();

        switch (which)
        {
        case 1:
            return temp2;
            break;

        case 2:
            int temp1 = laexp->cal();
            return temp1 && temp2;
            break;
        }
        return 0;
    }
};
// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOExp : public Basenode
{
public:
    std::unique_ptr<Basenode> laexp;
    std::unique_ptr<Basenode> loexp;
    int which;
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return laexp->dumpcode();
            break;
        case 2:
            return help_tri_short(loexp.get(), laexp.get(), std::string("||"));
            break;
        }
        return "";
    }
    int cal()
    {
        int temp2 = laexp->cal();

        switch (which)
        {
        case 1:
            return temp2;
            break;

        case 2:
            int temp1 = loexp->cal();
            return temp1 || temp2;

            break;
        }
        return 0;
    }
};
// ConstExp      ::= Exp;
class Constexp : public Exp
{
public:
    std::unique_ptr<Basenode> exp;
    std::string dumpcode()
    {
        return exp->dumpcode();
    }
    int cal()
    {
        return exp->cal();
        return 0;
    }
};

inline std::string help_tri_short(Basenode *n1, Basenode *n2, const std::string &opname)
{
    static int logical = 0;
    std::string res = "result_" + std::to_string(logical++);
    auto create_zero = []()
    { auto n = std::make_unique<Number>(); n->num = "0"; return n; };
    auto create_one = []()
    { auto n = std::make_unique<Number>(); n->num = "1"; return n; };
    auto result = std::make_unique<Vardecl>(); // int result=
    auto bt = std::make_unique<Btype>();
    bt->t = "int";
    result->btype = std::move(bt);
    auto def = std::make_unique<Vardef>();
    def->ident = res;
    def->which = 2;
    auto st = std::make_unique<Stmt>(); // overall sentence
    auto stex = std::make_unique<Stmt>();
    auto eval = std::make_unique<EExp>();
    auto predic = std::make_unique<EExp>();
    eval->op = "!=";
    eval->eexp = std::unique_ptr<Basenode>(n2);
    eval->which = 2;
    eval->rexp = create_zero();
    predic->op = "==";
    predic->which = 2;
    predic->eexp = std::unique_ptr<Basenode>(n1);
    // 4. 智能指针不能共享对象！用 Lambda 表达式每次生成全新的数字节点

    if (opname == "||")
    {
        predic->rexp = create_zero();
        def->initval = create_one();
    }
    else
    {
        predic->rexp = create_one();
        def->initval = create_zero();
    }
    (result->vardef).push_back(std::move(def));
    result->dumpcode(false);
    stex->which = 1;
    auto i = std::make_unique<Lval>();
    i->ident = res;
    stex->lval = std::move(i);
    stex->exp = std::move(eval);
    st->which = 5;
    st->exp = std::move(predic);
    st->stmt = std::move(stex);
    st->optstmt = nullptr;
    st->process_if_while();
    EExp *p = static_cast<EExp *>(st->exp.get());
    p->eexp.release();

    // eval 藏在 st->stmt->exp 中，它拥有 n2
    Stmt *sx = static_cast<Stmt *>(st->stmt.get());
    EExp *e = static_cast<EExp *>(sx->exp.get());
    e->eexp.release();
    // 修复 3：将计算结果 load 到临时寄存器中，并返回这个寄存器！
    // 这样外层如果是赋值运算，拿到 "%3" 就可以直接 store 了。
    std::string ptr_name = varTable->find(res)->val;
    std::string reg = "%" + std::to_string(getreg());
    std::cout << "    " << reg << " = load %" << ptr_name << std::endl;
    increg();

    return reg; // 完美返回寄存器
}