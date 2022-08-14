#include "Limit.h"
#include <iostream>

Limit::Limit(Order* order){
    limit_price = order->get_price();
    head = order;
    tail = order;
    depth = order->get_volume();
}


Limit::~Limit(){}


void Limit::add_order(Order* order) {
    tail->update_next(order);
    tail = order;
    depth += tail->get_volume();
}


void Limit::to_std_output(){
    std::cout<< depth << " available at limit price " << limit_price << std::endl;   
}


int Limit::get_depth_at_limit(){
    return depth;
}


int Limit::get_limit_price(){
    return limit_price;
}