#include "../Order/Order.h"
#pragma once

class Limit
{
private:
    int limit_price;
    int depth;
    Order* head;
    Order* tail;
    Limit* left_limit;
    Limit* right_limit;
public:
    Limit(Order* order);
    ~Limit();
    void add_order(Order* order);
    void to_std_output();
    int get_depth_at_limit();
    int get_limit_price();
};

