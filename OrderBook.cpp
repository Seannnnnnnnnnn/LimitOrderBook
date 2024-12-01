#include <iostream>
#include <numeric>
#include <chrono>
#include "OrderBook.h"


void OrderBook::pruneGoodForDayOrders()
{
    const auto end = std::chrono::hours(16);  // prune GoodForDay orders at 4pm

    // TODO: add in checks around time for order pruning
    
    while (true)
    {
        OrderIds orderIds;
        {
            std::scoped_lock( ordersMutex );

            for (const auto& [_, entry] : orders_) 
            {
                const auto& [order, _] = entry;

                if (order->GetOrderType() == OrderType::GoodForDay)
                    orderIds.push_back(order->GetOrderId());
            }
        }
        CancelOrders(orderIds);        
    }
}


void OrderBook::CancelOrders(OrderIds orderIds)
{
    // Implement a private method for cancelling orders to avoid excessive memory buss traffic
    // when pruning good for day orders. 
    std::scoped_lock { ordersMutex }; 
    
    for (const auto& orderId : orderIds)
    {
        CancelOrderInternal(orderId);
    }
}


void OrderBook::CancelOrderInternal(OrderId orderId)
{
    if (!orders_.contains(orderId)) 
        return;
    
    const auto& [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->GetSide() == Side::Sell)
    {
        auto price = order->GetPrice();
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            asks_.erase(price);
    }
    else
    {
        auto price = order->GetPrice();
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            bids_.erase(price);
    }
}


bool OrderBook::CanMatch(Side side, Price price) const 
{
    // Used for Fill and Kill orders to determine if the order can be matched. 
    // If false, Fill and Kill orders can just be discarded in constant time.  
    {
        if (side == Side::Buy)
        {
            if (asks_.empty()) {
                return false;
            
            const auto& [bestAsk, _] = *asks_.begin();
            return price >= bestAsk;
            }
        }
        else
        {
            if (bids_.empty()) {
                return false;
            }

            const auto& [bestBid, _] = *bids_.begin();
            return price <= bestBid;
        }
        return true;  // avoid compiler warning - matching will result in an empty list anyways
    }
}


Trades OrderBook::MatchOrders()
{
    Trades trades;
    trades.reserve(orders_.size());

    while (true) {
        
        if (bids_.empty() || asks_.empty()) {
            break;
        }

        auto& [bidPrice, bids] = *bids_.begin();
        auto& [askPrice, asks] = *asks_.begin();

        if (bidPrice < askPrice) {
            break;
        }

        while (bids.size() && asks.size()) {
            auto& bid = bids.front();
            auto& ask = asks.front();

            Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            bid->Fill(quantity);
            ask->Fill(quantity);

            if (bid->IsFilled()) {
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
            }

            if (ask->IsFilled()) {
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
            }

            if (bids.empty()) {
                bids_.erase(bidPrice);
            }

            if (asks.empty()) {
                asks_.erase(askPrice);
            }

            trades.push_back(Trade{
                TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity }, 
                TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity }
                });
        }
    }
    // For Fill and Kill orders - if it's not fully filled we need to remove it from the Order Book. 
    if (!bids_.empty()) {
        auto& [_, bids] = *bids_.begin();
        auto& order = bids.front();
        if (order->GetOrderType() == OrderType::FillAndKill) {
            CancelOrder(order->GetOrderId()); 
        }
    }
    if (!asks_.empty()) {
        auto& [_, asks] = *asks_.begin();
        auto& order = asks.front();
        if (order->GetOrderType() == OrderType::FillAndKill) {
            CancelOrder(order->GetOrderId()); 
        }
    }
    return trades;
}


Trades OrderBook::AddOrder(OrderPointer order)
{
    if (orders_.contains(order->GetOrderId())) {
        std::cout<< "Rejecting Order : " << order->GetOrderId() << ". Already present in Order Book." << std::endl;
        return { };
    }

    if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice())) {
        return { };
    }

    if (order->GetOrderType() == OrderType::Market)
    {
        // To handle market orders we first check that there is volume on opposite side of the book
        // Provided there is volume, transform the order to a Good Till Cancel on the worst price they 
        // can be filled at.
        if (order->GetSide() == Side::Sell && !asks_.empty()) 
        {
            const auto& [worstAsk, _] = *asks_.rbegin();
            order->ToGoodTillCancel(worstAsk);
        }

        else if (order->GetSide() == Side::Buy && !bids_.empty())
        {
            const auto& [worstBid, _]  = *bids_.rbegin();
            order->ToGoodTillCancel(worstBid);
        }

        // no volume on other side, so return empty set of trades.
        else 
        {
            return { };
        }
    }

    OrderPointers::iterator iterator;

    if (order->GetSide() == Side::Buy) {
        auto& orders = bids_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::next(orders.begin(), orders.size()-1);
    }
    else {
        auto& orders = asks_[order->GetPrice()];
        orders.push_back(order);
        iterator = std::next(orders.begin(), orders.size()-1);
    }

    orders_.insert({ order->GetOrderId(), OrderEntry{ order, iterator }});
    return MatchOrders();
}


void OrderBook::CancelOrder(OrderId orderId)
{
    if (!orders_.contains(orderId)) {
        return;
    }

    const auto& [order, orderIterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->GetSide() == Side::Sell) {
        Price price = order->GetPrice();
        auto& orders = asks_.at(price);
        orders.erase(orderIterator); 

        if (orders.empty()) {
            asks_.erase(price); 
        }
    }

    else {
        Price price = order->GetPrice();
        auto& orders = bids_.at(price);
        orders.erase(orderIterator); 
        if (orders.empty()) {
            asks_.erase(price); 
        }
    }
}


Trades OrderBook::ModifyOrder(OrderModify order)
{
    if (!orders_.contains(order.GetOrderId())) {
        return { };
    }
    
    const auto& [existing_order, _] = orders_.at(order.GetOrderId());
    CancelOrder(existing_order->GetOrderId());
    return AddOrder(order.ToOrderPointer(existing_order->GetOrderType()));
}

OrderBookLevelInfos OrderBook::GetOrderInfos() const
{
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(orders_.size());
    askInfos.reserve(orders_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders) 
    {
        return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0, 
                [](Quantity runningSum, const OrderPointer& order)
                { return runningSum + order->GetRemainingQuantity(); }) };
    };

    for (const auto& [price, orders] : bids_) {
        bidInfos.push_back(CreateLevelInfos(price, orders));
    }

    for (const auto& [price, orders] : asks_) {
        askInfos.push_back(CreateLevelInfos(price, orders));
    }
    return OrderBookLevelInfos{ bidInfos, askInfos };
}


std::size_t OrderBook::Size() const
{ 
    return orders_.size(); 
}