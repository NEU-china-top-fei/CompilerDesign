#pragma once
#include<memory>
#include<string>
#include<iostream>
class Basenode{
    public:
    virtual ~Basenode()=default;
    virtual void dump()=0;
    virtual void dumpcode()=0;
};
class CompUnit:public Basenode{
    public:
    std::unique_ptr<Basenode> func_def;
    void dump(){
        std::cout<<"Compuit { ";
        func_def->dump();
        std::cout<<" } ";
    }
    void dumpcode(){
        func_def->dumpcode();
    }
};
class Funcdef:public Basenode{
    public:
    std::unique_ptr<Basenode> func_type;
    std::string ident;
    std::unique_ptr<Basenode> block;
    void dump(){
        std::cout<<" Funcdef { ";
        func_type->dump();
        std::cout<<ident;
        block->dump();
        std::cout<<" } ";
    }
    void dumpcode(){
        std::cout<<"fun @"<<ident<<"(): ";
        func_type->dumpcode();
        std::cout<<" {"<<std::endl;
        block->dumpcode();
        std::cout<<std::endl<<"}";
    }
};
class Functype:public Basenode{
    public:
    std::string tpname;
    void dump(){
        std::cout<<" Functype { ";
        std::cout<<tpname;
        std::cout<<" } ";
    }
    void dumpcode(){
        if(tpname=="int"){
            std::cout<<"i32";
        }
    }
};
class Block:public Basenode{
    public:
    std::unique_ptr<Basenode> stmt;
    void dump(){
        std::cout<<" Block { ";
        stmt->dump();
        std::cout<<" } ";
    }
    void dumpcode(){
        std::cout<<"\%entry:"<<std::endl;
        stmt->dumpcode();
    }
};
class Stmt:public Basenode{
    public:
    std::unique_ptr<Basenode> number;
    void dump(){
        std::cout<<" Stmt { ";
        number->dump();
        std::cout<<" } ";
    }
    void dumpcode(){
        std::cout<<"ret ";
        number->dumpcode();
    }
};
class Number:public Basenode{
    public:
    std::string num;
    void dump(){
        std::cout<<num;
    }
    void dumpcode(){
        std::cout<<num;
    }
};