#pragma once

#include <memory>
#include <format>
#include "Usings.h"
#include "OrderType.h"
#include "Side.h"


// Used for market orders in which we orders effectively have an "invalid price" as we 
// only want to fill up to the required quantity
static const Price InvalidPrice = std::numeric_limits<Price>::quiet_NaN();


class Order
{
public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : orderType_{ orderType }
        , orderId_{ orderId }
        , side_{ side }
        , price_{ price }
        , initialQuantity_{ quantity }
        , remainingQuantity_{ quantity }
    { }

    Order(OrderId orderId, Side side, Quantity quantity)
        : Order(OrderType::Market, orderId, side, InvalidPrice, quantity)
    { }
    
    OrderType GetOrderType() const { return orderType_; }
    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
    bool IsFilled() const { return GetRemainingQuantity() == 0; }

    void Fill(Quantity quantity)
    {
        if (quantity > GetRemainingQuantity()) 
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than remaining quantity", GetOrderId()));
        
        remainingQuantity_ -= quantity;
    }

    // Transforms a OrderType::Market order into a Good Till Cancel via the OrderBook::AddOrder() method
    void ToGoodTillCancel(Price price)
    {
        if (GetOrderType() != OrderType::Market) 
            throw std::logic_error(std::format("Order ({}) cannot be transformed to Good Till Cancel. Only Orders of Type 'Market' can.", GetOrderId())); 
        
        price_ = price;
        orderType_ = OrderType::GoodTillCancel;
    }

    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
};

// NOTE: we use a list for O(1) cancels
using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;