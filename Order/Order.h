#pragma once

class Order
{
private:
    char position; 
    int volume;
    int price;
    bool filled;
    bool cancelled;
    Order* next_order;   // we treat Order as a node in a double link list
public:
    Order(char pos, int vol, int price);
    ~Order();
    void cancel();
    void to_std_output();
    int get_price();
    int get_volume();
    void update_next(Order* order);
};