#include <iostream>
#include "Order/Order.h"
#include "Limit/Limit.h"


int main(int argc, char const *argv[])
{
    Order order = Order('b', 10, 15);
    order.to_std_output();
    Order order2 = Order('b', 52, 15);
    Limit limit = Limit(&order);
    limit.to_std_output();
    limit.add_order(&order2);
    limit.to_std_output();
    return 0;
}
