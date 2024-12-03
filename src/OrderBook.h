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


    // Stores meta data about each price level
    struct LevelData
        {
            Price price_;
            Quantity quantity_;

            enum class Action
            {
                Add,
                Remove,
                Match,
            };
        };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    std::unordered_map<Price, LevelData> priceLevelMetaData_;

    std::mutex ordersMutex;
    std::thread ordersPruneThread;

    void pruneGoodForDayOrders();
    void CancelOrders(OrderIds orderIds); 
    void CancelOrderInternal(OrderId orderId);

    void OnOrderCancelled(OrderPointer order);
    void OnOrderAdded(OrderPointer order);
    void OnOrderMatched(OrderPointer order);
    void UpdateLevelData();

    bool CanFullyFill(Price price, Quantity quantity, Side side) const;
    bool CanMatch(Side side, Price price) const;
    Trades MatchOrders();

public:

    Trades AddOrder(OrderPointer order);
    void CancelOrder(OrderId OrderId);
    Trades ModifyOrder(OrderModify order);
    OrderBookLevelInfos GetOrderInfos() const;
    std::size_t Size() const;
};

