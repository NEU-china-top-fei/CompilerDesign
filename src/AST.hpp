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
#define setreg() (reg_cnt = 0)
#define increg() (reg_cnt++)
#define getreg() (reg_cnt) // 返回当前语句的目标寄存器
#define decreg(x) (reg_cnt -= x)
extern std::map<std::string, std::string> name2op;
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
    virtual std::string dumpcode(std::string &ident) { return std::string(""); }
    virtual std::string dumpcode(Basenode *btype) { return std::string(""); }
    virtual std::string dumpcode(bool judge) { return std::string(""); }
    virtual int cal() { return 0; }
    virtual std::string addr() { return std::string(""); }
    virtual bool bdumpcode() { return false; }
};

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

    return varTable->find(temp);
}

inline std::string help_tri(Basenode *n1, Basenode *n2, const std::string &opname)
{
    std::string str1 = n1->dumpcode();
    std::string str2 = n2->dumpcode();
    std::string op = name2op[opname];

    if (op == "and" || op == "or")
    {
        int first = getreg();
        increg();
        int second = getreg();
        increg();
        std::cout << "    %" << first << " = ne " << str1 << " ,0" << std::endl;
        std::cout << "    %" << second << " = ne " << str2 << " ,0" << std::endl;
        str1 = "%" + std::to_string(first);
        str2 = "%" + std::to_string(second);
    }

    std::string temp = "%" + std::to_string(getreg());
    increg();

    std::cout << "    " << temp << " = " << op << " " << str1 << " , " << str2 << std::endl;
    return temp;
}
// CompUnit:= FuncDef
class CompUnit : public Basenode
{
public:
    std::unique_ptr<Basenode> func_def;
    void dump()
    {
        std::cout << "Compuit { ";
        func_def->dump();
        std::cout << " } ";
    }
    std::string dumpcode()
    {
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
        func_def->dumpcode();
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
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            constde->dumpcode();
            break;

        case 2:
            varde->dumpcode();
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
    std::string dumpcode()
    {
        for (auto i = constdef.begin(); i != constdef.end(); i++)
        {
            (*i)->dumpcode();
        }
        return "";
    }
};
// BType :: = "int";
class Btype : public Basenode
{
public:
    std::string t;
    std::string dumpcode()
    {
        if (t == "int")
            std::cout << " i32";
        return "";
    }
};
// ConstDef :: = IDENT "=" ConstInitVal;
class Constdef : public Basenode
{
public:
    std::string ident;
    std::unique_ptr<Basenode> constinit;
    std::string dumpcode()
    {

        constinit->dumpcode(ident);
        return "";
    }
};
// ConstInitVal :: = ConstExp;
class Constinit : public Basenode
{
public:
    std::unique_ptr<Basenode> cexp;
    std::string dumpcode(std::string &ident)
    {
        int result = cexp->cal();
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
    std::string dumpcode()
    {
        for (auto i = vardef.begin(); i != vardef.end(); i++)
        {
            (*i)->dumpcode(btype.get());
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
    std::string dumpcode(Basenode *btype)
    {
        std::string dis_tag = std::to_string(varTable->layercnt);
        std::cout << "    %" << ident << dis_tag << "  = alloc" << btype->dumpcode() << std::endl;
        varTable->add(ident, ident + dis_tag);
        if (which == 2)
        {
            std::string temp = initval->dumpcode();
            std::cout << "    store " << temp << ", %" << ident + dis_tag << std::endl;
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
//  FuncDef:= Functype IDENT "("  ")" Block
class Funcdef : public Basenode
{
public:
    std::unique_ptr<Basenode> func_type;
    std::string ident;
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
        std::cout << "fun @" << ident << "(): ";
        func_type->dumpcode();
        std::cout << " {" << std::endl;
        std::cout << "%entry:" << std::endl;
        block->bdumpcode();
        std::cout << std::endl
                  << "}";
        return "";
    }
};
// Functype:="int"
class Functype : public Basenode
{
public:
    std::string tpname;
    void dump()
    {
        std::cout << " Functype { ";
        std::cout << tpname;
        std::cout << " } ";
    }
    std::string dumpcode()
    {
        if (tpname == "int")
        {
            std::cout << "i32";
        }
        return "";
    }
};
// Block         ::= "{" {BlockItem} "}";
// BlockItem     ::= Decl | Stmt;
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
            // std::cerr << "Processing BlockItem!" << std::endl; // 埋点
            isret = isret || (*i)->bdumpcode();
        }
        if (!isret)
        {
            std::cout << "    ret 0" << std::endl;
        }
        constTable = constTable->parent;
        varTable = varTable->parent;
        return true;
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
        // std::cerr << "Processing BlockItem! which = " << which << std::endl; // 重点打印 which
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
//[]    refers to repeat zero or more times
class Stmt : public Basenode
{
public:
    std::unique_ptr<Basenode> lval;
    std::unique_ptr<Basenode> exp;
    std::unique_ptr<Basenode> block;
    std::unique_ptr<Basenode> optexp;
    int which;
    bool bdumpcode()
    {
        // std::cerr << "Stmt which = " << which << std::endl;
        switch (which)
        {
        case 1:
        {
            std::string value = exp->dumpcode();
            std::string ptr = lval->dumpcode(true);
            std::cout << "    store " << value << ", " << ptr << std::endl;
            break;
        }
        case 2:
            if (optexp != nullptr)
                optexp->dumpcode();
            break;
        case 3:
            return block->bdumpcode();
            break;
        // case 4:
        // {
        //     std::string temp = "0";
        //     if (optexp != nullptr)
        //         temp = optexp->dumpcode();
        //     std::cout << "    ret " << temp << std::endl;
        //     return true;
        //     break;
        // }
        case 4:
        {
            std::string temp = "0";
            if (optexp != nullptr)
            {
                std::cerr << "Generating code for return expression..." << std::endl;
                temp = optexp->dumpcode();
                std::cerr << "Return expression dumpcode returned: '" << temp << "'" << std::endl;
            }
            else
            {
                std::cerr << "Return with no expression" << std::endl;
            }
            std::cout << "    ret " << temp << std::endl;
            std::cerr << "Generated: ret " << temp << std::endl;
            return true;
            break;
        }
        }

        return false;
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
// class Exp : public Basenode
// {
// public:
//     std::unique_ptr<Basenode> loexp;
//     std::string dumpcode()
//     {
//         return loexp->dumpcode();
//     }
//     int cal()
//     {
//         return loexp->cal();
//     }
// };
class Exp : public Basenode
{
public:
    std::unique_ptr<Basenode> loexp;

    std::string dumpcode() override
    {
        std::cerr << "[DEBUG] Exp::dumpcode() this=" << this << std::endl;
        std::cerr << "[DEBUG]   loexp ptr: " << loexp.get() << std::endl;
        if (loexp)
        {
            std::cerr << "[DEBUG]   loexp type: " << demangle(typeid(*loexp).name()) << std::endl;
        }
        return loexp->dumpcode();
    }
};
class Lval : public Basenode
{
public:
    std::string ident;
    std::string addr()
    {
        return std::string("%") + (*varTable->find(ident));
    }
    // std::string dumpcode(bool is_addr)
    // {
    //     if (is_addr)
    //     {
    //         return std::string("%") + (*varTable->find(ident));
    //     }

    //     auto c = constTable->find(ident);
    //     if (c != nullptr)
    //         return std::to_string(*c);

    //     auto v = varTable->find(ident);
    //     if (v != nullptr)
    //     {
    //         std::string reg = "%" + std::to_string(getreg());
    //         std::cout << "    " << reg << " = load %" << *v << std::endl;
    //         increg();
    //         return reg;
    //     }
    //     return "0";
    // }
    std::string dumpcode(bool is_addr)
    {
        std::cerr << "Lval::dumpcode(" << is_addr << ") for ident: " << ident << std::endl;

        if (is_addr)
        {
            auto v = varTable->find(ident);
            if (v)
            {
                std::cerr << "  Found address: %" << *v << std::endl;
                return std::string("%") + (*v);
            }
            std::cerr << "  Address not found!" << std::endl;
            return "%0";
        }

        auto c = constTable->find(ident);
        if (c != nullptr)
        {
            std::cerr << "  Found as constant: " << *c << std::endl;
            return std::to_string(*c);
        }

        auto v = varTable->find(ident);
        if (v != nullptr)
        {
            std::cerr << "  Found as variable: %" << *v << std::endl;
            std::string reg = "%" + std::to_string(getreg());
            std::cout << "    " << reg << " = load %" << *v << std::endl;
            std::cerr << "  Generated load to " << reg << std::endl;
            increg();
            return reg;
        }
        std::cerr << "  Variable not found in any table!" << std::endl;
        return "0";
    }
    int cal() override
    {
        auto c = constTable->find(ident);
        if (c != nullptr)
            return *c;

        return 0;
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
        std::cerr << "PEXP ,which=" << which << std::endl;
        switch (which)
        {
        case 1:
            std::cerr << "PExp calling which=1" << std::endl;
            return exp->dumpcode();
            break;
        case 2:
            std::cerr << "PExp calling which=2" << std::endl;
            return lval->dumpcode(false);
        case 3:
            std::cerr << "PExp calling which=3" << std::endl;
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

// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
class UExp : public Basenode
{
public:
    std::unique_ptr<Basenode> pexp;
    std::unique_ptr<Basenode> uop;
    std::unique_ptr<Basenode> uexp;
    int which;

    UExp()
    {
        std::cerr << "UExp constructor, this=" << this << std::endl;
    }

    ~UExp()
    {
        std::cerr << "UExp destructor, this=" << this << std::endl;
    }

    // 禁用拷贝构造和赋值
    UExp(const UExp &) = delete;
    UExp &operator=(const UExp &) = delete;

    // 移动构造函数
    UExp(UExp &&other) noexcept
        : pexp(std::move(other.pexp)),
          uop(std::move(other.uop)),
          uexp(std::move(other.uexp)),
          which(other.which)
    {
        std::cerr << "UExp move constructor, this=" << this << ", from=" << &other << std::endl;
    }

    // 移动赋值
    UExp &operator=(UExp &&other) noexcept
    {
        std::cerr << "UExp move assignment, this=" << this << ", from=" << &other << std::endl;
        if (this != &other)
        {
            pexp = std::move(other.pexp);
            uop = std::move(other.uop);
            uexp = std::move(other.uexp);
            which = other.which;
        }
        return *this;
    }

    std::string dumpcode() override
    {
        std::cerr << "UExp::dumpcode() this=" << this << ", which=" << which << std::endl;
        std::cerr << "  pexp ptr: " << pexp.get() << std::endl;
        if (pexp)
        {
            std::cerr << "  pexp type: " << typeid(*pexp).name() << std::endl;
        }

        switch (which)
        {
        case 1:
            return pexp->dumpcode();
            break;
        case 2:
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
// class UExp : public Basenode
// {
// public:
//     std::unique_ptr<Basenode> pexp;
//     std::unique_ptr<Basenode> uop;
//     std::unique_ptr<Basenode> uexp;
//     int which;
//     std::string dumpcode()
//     {
//         switch (which)
//         {
//         case 1:
//             return pexp->dumpcode();
//             break;
//         case 2:
//         {
//             std::string temp = uexp->dumpcode();

//             if (uop->retop() == "+")
//                 return temp;

//             if (uop->retop() == "-")
//             {
//                 std::string reg = "%" + std::to_string(getreg());
//                 std::cout << "    " << reg << " = sub 0, " << temp << std::endl;
//                 increg();
//                 return reg;
//             }

//             if (uop->retop() == "!")
//             {
//                 std::string reg = "%" + std::to_string(getreg());
//                 std::cout << "    " << reg << " = eq " << temp << ", 0" << std::endl;
//                 increg();
//                 return reg;
//             }
//         }

//         break;
//         }
//         return "";
//     }
//     int cal()
//     {

//         switch (which)
//         {
//         case 1:
//             return pexp->cal();
//             break;

//         case 2:
//             int temp = uexp->cal();
//             if (uop->retop() == "-")
//             {
//                 return -1 * temp;
//             }
//             else if (uop->retop() == "!")
//             {
//                 return temp == 0;
//             }
//             else if (uop->retop() == "+")
//             {
//                 return temp;
//             }
//             break;
//         }
//         return 0;
//     }
// };
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
// MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
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
            return help_tri(laexp.get(), eexp.get(), std::string("&&"));
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
            return help_tri(loexp.get(), laexp.get(), std::string("||"));
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