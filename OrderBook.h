#pragma once

#include <map>
#include <unordered_map>
#include <thread>

#include "Usings.h"
#include "Order.h"
#include "Trade.h"
#include "OrderModify.h"
#include "OrderBookLevelInfos.h"

class OrderBook
{
private:
    struct OrderEntry
        {
            OrderPointer order_{ nullptr };
            OrderPointers::iterator location_;
        };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    std::mutex ordersMutex;
    std::thread ordersPruneThread;

    bool CanMatch(Side side, Price price) const;
    Trades MatchOrders();

    void pruneGoodForDayOrders();
    void CancelOrders(OrderIds orderIds); 
    void CancelOrderInternal(OrderId orderId);

public:
    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId OrderId);
    Trades ModifyOrder(OrderModify order);
    OrderBookLevelInfos GetOrderInfos() const;
    std::size_t Size() const;
};

