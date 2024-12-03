#include <vector>
#include <exception>
#include <iostream>
#include <memory>
#include <map>
#include <unordered_map>
#include <list>
#include <format>
#include <numeric>

#include "OrderBook.h"


int main()
{
    // lil testing
    OrderBook orderbook;
    const OrderId orderId = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    std::cout<< orderbook.Size() << std::endl;
    orderbook.CancelOrder(1);
    std::cout<< orderbook.Size() << std::endl;
    std::cout<<"Refactored officially" << std::endl;
    return 0;
}
