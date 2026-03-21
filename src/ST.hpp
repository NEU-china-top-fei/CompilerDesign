#pragma once
#include <map>
#include <memory>
#include <variant>
// header for symbol table
extern int globalcnt;
template <typename t>
class ST
{
public:
    std::map<std::string, t> core;
    ST *parent;
    ST()
    {
        parent = nullptr;
    }
    ST *enter_scope(bool isfirst = true)
    {
        auto new_scope = new ST();
        new_scope->parent = this;
        return new_scope;
    }
    int getcnt()
    {
        return globalcnt++;
    }
    bool add(std::string &key, t item)
    {
        if (core.find(key) != core.end())
            return false;

        core[key] = item;
        return true;
    }
    t *find(std::string &key)
    {
        auto cur_table = this;
        while (cur_table != nullptr)
        {
            auto it = cur_table->core.find(key);
            if (it != cur_table->core.end())
            {
                return &(it->second);
            }
            cur_table = cur_table->parent;
        }
        return nullptr;
    }
    bool check_if_defind(std::string &key)
    {
        return core.find(key) != core.end();
    }
    // exit automatically
};
inline ST<int> *constTable;
inline ST<std::string> *varTable;
