#include "Order.h"
#include <stdexcept>
#include <iostream>


Order::Order(char pos, int vol, int limit) {
    if (pos != 'b' && pos != 'a'){
        throw std::invalid_argument("invalid position argument. Position can only be 'b' or 'a'");
    }    
    position = pos;
    volume = vol;
    price = limit;
}


Order::~Order(){}


void Order::cancel(){
    volume = 0;
    cancelled = true;
}


void Order::to_std_output(){
    std::cout<< position << " " << volume << " @ " << price << std::endl;
}


int Order::get_price(){
    return price;
}


int Order::get_volume(){
    return volume;
}


void Order::update_next(Order* order){
    next_order = order;
}