/* We represent each side of the book as a Binary tree of limits */ 
#include "../Order/Order.h"
#include "../Limit/Limit.h"
#include <map>
#pragma once


class Tree
{
private:
    std::map<int, Limit*> limit_map;
    Limit* root;
public:
    Tree();
    ~Tree();
    void insert(Limit*);
};

