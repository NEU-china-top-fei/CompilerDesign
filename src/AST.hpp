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

extern ST<int> *constTable;
extern ST<ele> *varTable;
extern ST<std::string> *funcTable;
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
    virtual void dump(int depth = 0) {};
    virtual std::string dumpcode() { return std::string(""); };
    virtual std::string retop() { return std::string(""); };
    virtual std::string dumpcode(std::string &ident, bool isglobal) { return std::string(""); }
    virtual std::string dumpcode(Basenode *btype, bool isglobal) { return std::string(""); }
    virtual std::string dumpcode(bool judge) { return std::string(""); }
    virtual int cal() { return 0; }
    virtual std::string addr() { return std::string(""); }
    virtual bool bdumpcode() { return false; }

protected:
    void printIdent(int depth)
    {
        for (int i = 0; i < depth; i++)
        {
            std::cout << "  ";
        }
    }
};

inline std::string help_tri_short(Basenode *n1, Basenode *n2, const std::string &opname);

/**
 * given a string,
 * judge if immediate(e.g:9),register(%),or defined (const)variable
 * return variable name
 */
inline std::string *process_variable(std::string &temp)
{
    if (temp.empty())
        return nullptr;
    char prefix = temp.c_str()[0];
    bool isdigit = prefix >= '0' && prefix <= '9';
    if (prefix == '%')
        return nullptr;
    if (isdigit)
        return nullptr;
    auto c = constTable->find(temp);
    if (c)
    {
        return new std::string(std::to_string(*c));
    }

    auto v = varTable->find(temp);
    if (v == nullptr)
        return nullptr;
    if (!v->is_ptr)
        return &(v->val);

    return nullptr;
}

/**
 * helper method for binary operator
 * return the register of result
 */
inline std::string help_tri(Basenode *n1, Basenode *n2, const std::string &opname)
{
    std::string str1 = n1->dumpcode();
    std::string str2 = n2->dumpcode();
    std::string op = name2op[opname];
    std::string temp = "%" + std::to_string(getreg());
    increg();

    std::cout << "    " << temp << " = " << op << " " << str1 << " , " << str2 << std::endl;
    return temp;
}

class CompUnit : public Basenode
{
public:
    std::unique_ptr<Basenode> compunit;
    std::unique_ptr<Basenode> decl;
    std::unique_ptr<Basenode> func_def;
    int which;
    void dump(int depth = 0)
    {
        if (compunit)
        {
            compunit->dump(depth);
        }
        if (which == 1)
        {
            decl->dump(depth);
        }
        else
        {
            func_def->dump(depth);
        }
    }
    std::string dumpcode()
    {
        if (compunit != nullptr)
        {
            compunit->dumpcode();
        }
        if (which == 1)
            decl->dumpcode(true);
        else
        {
            func_def->dumpcode();
        }
        return "";
    }
};

class Decl : public Basenode
{
public:
    std::unique_ptr<Basenode> constde;
    std::unique_ptr<Basenode> varde;
    int which;
    void dump(int depth = 0)
    {
        switch (which)
        {
        case 1:
            constde->dump(depth);
            break;
        case 2:
            varde->dump(depth);
            break;
        }
    }
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

class ConstDecl : public Basenode
{
public:
    std::unique_ptr<Basenode> btype;
    std::vector<std::unique_ptr<Basenode>> constdef;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "ConstDecl:" << std::endl;
        btype->dump(depth + 1);
        for (auto &i : constdef)
        {
            i->dump(depth + 1);
        }
    }
    std::string dumpcode(bool isglobal)
    {
        for (auto i = constdef.begin(); i != constdef.end(); i++)
        {
            (*i)->dumpcode(isglobal);
        }
        return "";
    }
};

class Btype : public Basenode
{
public:
    std::string t;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "BType: " << t << std::endl;
    }
    std::string dumpcode()
    {
        return t;
    }
};

class Constdef : public Basenode
{
public:
    std::string ident;
    std::unique_ptr<Basenode> constinit;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "ConstDef: " << ident << std::endl;
        constinit->dump(depth + 1);
    }
    std::string dumpcode(bool isglobal)
    {
        constinit->dumpcode(ident, isglobal);
        return "";
    }
};

class Constinit : public Basenode
{
public:
    std::unique_ptr<Basenode> cexp;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "ConstInitVal: " << std::endl;
        cexp->dump(depth + 1);
    }
    std::string dumpcode(std::string &ident, bool isglobal)
    {
        int result = cexp->cal();
        if (isglobal)
            constTable->add_global(ident, result);
        else
            constTable->add(ident, result);
        return std::to_string(result);
    }
};

class Vardecl : public Basenode
{
public:
    std::unique_ptr<Basenode> btype;
    std::vector<std::unique_ptr<Basenode>> vardef;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "VarDecl:" << std::endl;
        btype->dump(depth + 1);
        for (auto &i : vardef)
        {
            i->dump(depth + 1);
        }
    }
    std::string dumpcode(bool isglobal)
    {
        for (auto i = vardef.begin(); i != vardef.end(); i++)
        {
            (*i)->dumpcode(btype.get(), isglobal);
        }
        return "";
    }
};

class Vardef : public Basenode
{
public:
    std::string ident;
    std::unique_ptr<Basenode> initval;
    int which;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "VarDef: " << ident << std::endl;
        if (which == 2)
        {
            initval->dump(depth + 1);
        }
    }
    std::string dumpcode(Basenode *btype, bool isglobal)
    {
        std::string dis_tag = std::to_string(varTable->getcnt());
        std::string mapped_name = ident + dis_tag;
        std::string temp = "";

        if (isglobal)
        {
            std::cout << "global %" << mapped_name << "  = alloc " << btype->dumpcode();

            if (which == 2)
            {
                int val = initval->cal();
                temp = std::to_string(val);
                std::cout << " , " << temp << std::endl;
            }
            else
            {
                std::cout << " , zeroinit" << std::endl;
            }
            ele newe;
            newe.val = mapped_name;
            newe.is_ptr = true;
            varTable->add_global(ident, newe);
        }
        else
        {
            std::cout << "    %" << mapped_name << "  = alloc " << btype->dumpcode() << std::endl;
            ele new_e;
            new_e.is_ptr = true;
            new_e.val = mapped_name;
            varTable->add(ident, new_e);

            if (which == 2)
            {
                temp = initval->dumpcode();
                std::cout << "    store " << temp << ", %" << new_e.val << std::endl;
            }
        }
        return "";
    }
};

class Initval : public Basenode
{
public:
    std::unique_ptr<Basenode> exp;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "InitVal: " << std::endl;
        exp->dump(depth + 1);
    }
    std::string dumpcode()
    {
        return exp->dumpcode();
    }
    int cal()
    {
        return exp->cal();
    }
};

class Funcdef : public Basenode
{
public:
    std::unique_ptr<Basenode> func_type;
    std::string ident;
    std::unique_ptr<Basenode> funcfparams;
    std::unique_ptr<Basenode> block;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "FuncDef: " << ident << std::endl;
        func_type->dump(depth + 1);
        if (funcfparams)
        {
            printIdent(depth + 1);
            std::cout << "Params: " << std::endl;
            funcfparams->dump(depth + 2);
        }
        block->dump(depth + 1);
    }
    std::string dumpcode()
    {
        setreg();
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
            if (temp == "i32")
                std::cout << "0";
            std::cout << std::endl;
        }

        std::cout << "}" << std::endl;
        varTable = varTable->parent;
        return "";
    }
};

class Funcfparams : public Basenode
{
public:
    std::vector<std::unique_ptr<Basenode>> funcfparams;
    void dump(int depth = 0)
    {
        for (auto &i : funcfparams)
        {
            i->dump(depth);
        }
    }
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

class Funcfparam : public Basenode
{
public:
    std::unique_ptr<Basenode> btype;
    std::string ident;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "FuncFParam: " << ident << std::endl;
        btype->dump(depth + 1);
    }
    std::string dumpcode()
    {
        std::cout << "%" << ident << " : " << btype->dumpcode();
        return ident;
    }
};

class Block : public Basenode
{
public:
    std::vector<std::unique_ptr<Basenode>> blockitem;
    void dump(int depth = 0) override
    {
        printIdent(depth);
        std::cout << "Block {" << std::endl;
        for (const auto &item : blockitem)
        {
            item->dump(depth + 1); // 里面的语句深度 +1
        }
        printIdent(depth);
        std::cout << "}" << std::endl;
    }
    bool bdumpcode()
    {
        constTable = constTable->enter_scope();
        varTable = varTable->enter_scope();
        bool isret = false;
        for (auto i = blockitem.begin(); i != blockitem.end(); i++)
        {
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
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            decl->dump(depth);
        }
        else
        {
            stmt->dump(depth);
        }
    }
    bool bdumpcode()
    {
        switch (which)
        {
        case 1:
            decl->dumpcode(false);
            break;
        case 2:
            return stmt->bdumpcode();
        }
        return false;
    }
};

class Stmt : public Basenode
{
public:
    std::unique_ptr<Basenode> lval;
    std::unique_ptr<Basenode> exp;
    std::unique_ptr<Basenode> block;
    std::unique_ptr<Basenode> optexp;
    std::unique_ptr<Basenode> stmt;
    std::unique_ptr<Basenode> optstmt;
    void dump(int depth = 0)
    {
        switch (which)
        {
        case 1:
            printIdent(depth);
            std::cout << "AssignStmt:" << std::endl;
            lval->dump(depth + 1);
            exp->dump(depth + 1);
            break;
        case 2:
            if (optexp)
            {
                optexp->dump(depth);
            }
            else
            {
                printIdent(depth);
                std::cout << "EmptyStmt: ;" << std::endl;
            }
            break;
        case 4:
            printIdent(depth);
            std::cout << "ReturnStmt:" << std::endl;
            if (optexp)
                optexp->dump(depth + 1);
            break;
        case 5:
            printIdent(depth);
            std::cout << "IfStmt:" << std::endl;
            printIdent(depth + 1);
            std::cout << "Condition:" << std::endl;
            exp->dump(depth + 2);
            printIdent(depth + 1);
            std::cout << "Then:" << std::endl;
            stmt->dump(depth + 2);
            if (optstmt)
            {
                printIdent(depth + 1);
                std::cout << "Else:" << std::endl;
                optstmt->dump(depth + 2);
            }
            break;
        case 6:
            printIdent(depth);
            std::cout << "WhileStmt:" << std::endl;
            printIdent(depth + 1);
            std::cout << "Condition:" << std::endl;
            exp->dump(depth + 2);
            printIdent(depth + 1);
            std::cout << "Body:" << std::endl;
            stmt->dump(depth + 2);
            break;
        case 3:
            block->dump(depth);
            break;
        default:
            printIdent(depth);
            std::cout << "Stmt" << std::endl;
        }
    }
    void process_if_while(bool isloop = false)
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

        std::string target = this->optstmt == nullptr ? endi : elsei;
        if (judge)
        {
            std::cout << "    %" << getreg() << " = load %" << (*judge) << std::endl;
            std::cout << "    br %" << getreg() << "," << theni << "," << target << std::endl;
            increg();
        }
        else
        {
            std::cout << "    br " << predic << "," << theni << "," << target << std::endl;
        }

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
        }
        case 2:
            if (optexp != nullptr)
                optexp->dumpcode();
            return false;
        case 3:
            return block->bdumpcode();
        case 4:
        {
            if (optexp != nullptr)
            {
                // 有返回值（比如 return 1;）
                std::string temp = optexp->dumpcode();
                std::cout << "    ret " << temp << std::endl;
            }
            else
            {
                // 无返回值（比如 return;）
                std::cout << "    ret" << std::endl;
            }
            return true;
            break;
        }
        case 5:
            process_if_while();
            return false;
        case 6:
            process_if_while(true);
            return false;
        case 7:
            if (!break_tag.empty())
            {
                std::cout << "    jump " << break_tag.back() << std::endl;
                return true;
            }
            return false;
        case 8:
            if (!continue_tag.empty())
            {
                std::cout << "    jump " << continue_tag.back() << std::endl;
                return true;
            }
            return false;
        }
        return false;
    }
};

class Number : public Basenode
{
public:
    std::string num;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "Number: " << num << std::endl;
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

class Exp : public Basenode
{
public:
    std::unique_ptr<Basenode> loexp;
    void dump(int depth = 0)
    {
        loexp->dump(depth);
    }
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
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "LVal: " << ident << std::endl;
    }
    std::string addr()
    {
        return std::string("%") + (varTable->find(ident))->val;
    }
    int cal()
    {
        auto i = constTable->find(ident);
        if (i)
            return *i;
        return 0;
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

class PExp : public Basenode
{
public:
    std::unique_ptr<Basenode> exp;
    std::unique_ptr<Basenode> number;
    std::unique_ptr<Basenode> lval;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            exp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "PrimaryExp: " << std::endl;
            if (which == 2)
            {
                lval->dump(depth + 1);
            }
            else
            {
                number->dump(depth + 1);
            }
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return exp->dumpcode();
        case 2:
            return lval->dumpcode(false);
        case 3:
            return number->dumpcode();
        }
        return "";
    }
    int cal()
    {
        switch (which)
        {
        case 1:
            return exp->cal();
        case 2:
            return lval->cal();
        case 3:
            return number->cal();
        }
        return 0;
    }
};

class UExp : public Basenode
{
public:
    std::unique_ptr<Basenode> pexp;
    std::unique_ptr<Basenode> uop;
    std::string ident;
    std::unique_ptr<Basenode> funcrparams;
    std::unique_ptr<Basenode> uexp;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            pexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            if (which == 2)
            {
                std::cout << "UnaryExp: " << ident << std::endl;
                if (funcrparams)
                {
                    funcrparams->dump(depth + 1);
                }
            }
            else
            {
                std::cout << "UnaryExp: " << std::endl;
                uop->dump(depth + 1);
                uexp->dump(depth + 1);
            }
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return pexp->dumpcode();
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
                std::cout << "    " << reg << " = call @" << ident << "(" << args << ")" << std::endl;
            }
            else
                std::cout << "    call @" << ident << "(" << args << ")" << std::endl;
            return reg;
        }
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
        }
        return "";
    }
    int cal()
    {
        switch (which)
        {
        case 1:
            return pexp->cal();
        case 2:
            int temp = uexp->cal();
            if (uop->retop() == "-")
                return -1 * temp;
            else if (uop->retop() == "!")
                return temp == 0;
            else if (uop->retop() == "+")
                return temp;
            break;
        }
        return 0;
    }
};

class UOp : public Basenode
{
public:
    std::string op;
    int which;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "UnaryOp: " << op << std::endl;
    }
    std::string retop()
    {
        return op;
    }
};

class Funcrparams : public Basenode
{
public:
    std::vector<std::unique_ptr<Basenode>> explist;
    void dump(int depth = 0)
    {
        printIdent(depth);
        std::cout << "FuncRParams: " << std::endl;
        for (auto &i : explist)
        {
            i->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        std::ostringstream args;
        for (auto i = explist.begin(); i != explist.end(); i++)
        {
            args << (*i)->dumpcode();
            if ((*i) != explist.back())
            {
                args << ", ";
            }
        }
        return args.str();
    }
};

class MExp : public Basenode
{
public:
    std::unique_ptr<Basenode> uexp;
    std::string op;
    std::unique_ptr<Basenode> mexp;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            uexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "MulExp: " << op << std::endl;
            mexp->dump(depth + 1);
            uexp->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return uexp->dumpcode();
        case 2:
            return help_tri(mexp.get(), uexp.get(), op);
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
        case 2:
            int temp1 = mexp->cal();
            if (op == "*")
                return temp1 * temp2;
            else if (op == "/")
                return temp1 / temp2;
            else
                return temp1 % temp2;
        }
        return 0;
    }
};

class AExp : public Basenode
{
public:
    std::unique_ptr<Basenode> mexp;
    std::string op;
    std::unique_ptr<Basenode> aexp;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            mexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "AddExp: " << op << std::endl;
            aexp->dump(depth + 1);
            mexp->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return mexp->dumpcode();
        case 2:
            return help_tri(aexp.get(), mexp.get(), op);
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
        case 2:
            int temp1 = aexp->cal();
            if (op == "+")
                return temp1 + temp2;
            else if (op == "-")
                return temp1 - temp2;
        }
        return 0;
    }
};

class RExp : public Basenode
{
public:
    std::unique_ptr<Basenode> aexp;
    std::string op;
    std::unique_ptr<Basenode> rexp;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            aexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "RelExp: " << op << std::endl;
            rexp->dump(depth + 1);
            aexp->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return aexp->dumpcode();
        case 2:
            return help_tri(rexp.get(), aexp.get(), op);
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
        case 2:
            int temp1 = rexp->cal();
            if (op == "<")
                return temp1 < temp2;
            else if (op == ">")
                return temp1 > temp2;
            else if (op == "<=")
                return temp1 <= temp2;
            else if (op == ">=")
                return temp1 >= temp2;
        }
        return 0;
    }
};

class EExp : public Basenode
{
public:
    std::unique_ptr<Basenode> rexp;
    std::unique_ptr<Basenode> eexp;
    std::string op;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            rexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "EqExp: " << op << std::endl;
            eexp->dump(depth + 1);
            rexp->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return rexp->dumpcode();
        case 2:
            return help_tri(eexp.get(), rexp.get(), op);
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
        case 2:
            int temp1 = eexp->cal();
            if (op == "==")
                return temp1 == temp2;
            else if (op == "!=")
                return temp1 != temp2;
        }
        return 0;
    }
};

class LAExp : public Basenode
{
public:
    std::unique_ptr<Basenode> eexp;
    std::unique_ptr<Basenode> laexp;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            eexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "LAndExp: &&" << std::endl;
            eexp->dump(depth + 1);
            laexp->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return eexp->dumpcode();
        case 2:
            return help_tri_short(laexp.get(), eexp.get(), std::string("&&"));
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
        case 2:
            int temp1 = laexp->cal();
            return temp1 && temp2;
        }
        return 0;
    }
};

class LOExp : public Basenode
{
public:
    std::unique_ptr<Basenode> laexp;
    std::unique_ptr<Basenode> loexp;
    int which;
    void dump(int depth = 0)
    {
        if (which == 1)
        {
            laexp->dump(depth);
        }
        else
        {
            printIdent(depth);
            std::cout << "LOrExp: ||" << std::endl;
            laexp->dump(depth + 1);
            loexp->dump(depth + 1);
        }
    }
    std::string dumpcode()
    {
        switch (which)
        {
        case 1:
            return laexp->dumpcode();
        case 2:
            return help_tri_short(loexp.get(), laexp.get(), std::string("||"));
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
        case 2:
            int temp1 = loexp->cal();
            return temp1 || temp2;
        }
        return 0;
    }
};

class Constexp : public Exp
{
public:
    std::unique_ptr<Basenode> exp;
    void dump(int depth = 0)
    {
        exp->dump(depth);
    }
    std::string dumpcode()
    {
        return exp->dumpcode();
    }
    int cal()
    {
        return exp->cal();
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
    auto result = std::make_unique<Vardecl>();
    auto bt = std::make_unique<Btype>();
    bt->t = "i32";
    result->btype = std::move(bt);
    auto def = std::make_unique<Vardef>();
    def->ident = res;
    def->which = 2;
    auto st = std::make_unique<Stmt>();
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

    Stmt *sx = static_cast<Stmt *>(st->stmt.get());
    EExp *e = static_cast<EExp *>(sx->exp.get());
    e->eexp.release();

    std::string ptr_name = varTable->find(res)->val;
    std::string reg = "%" + std::to_string(getreg());
    std::cout << "    " << reg << " = load %" << ptr_name << std::endl;
    increg();

    return reg;
}