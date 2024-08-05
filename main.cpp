#include <vector>
#include <exception>
#include <iostream>
#include <memory>
#include <map>
#include <unordered_map>
#include <list>


using Price = int;
using Quantity = uint32_t;
using OrderId = uint32_t;
using OrderIds = std::vector<OrderId>;


enum class OrderType
{
    GoodTillCancel,
    FillAndKill,
};


enum class Side
{
    Buy,
    Sell,
};


struct LevelInfo
{
    Price price_;
    Quantity quantity;
};


using LevelInfos = std::vector<LevelInfo>;


class OrderBookLevelInfos
{
    public:
        OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
            : bids_{ bids }
            , asks_{ asks }
        { }

        const LevelInfos& GetBids() const { return bids_; }
        const LevelInfos& GetAsks() const { return asks_; }

    private:
        LevelInfos bids_;
        LevelInfos asks_;
};


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
        // filled for some invalid quantity
        if (quantity > GetRemainingQuantity()) {
            throw std::logic_error("Order ({}) cannot be filled for more than remaining quantity");
        }
        remainingQuantity_ -= quantity;
    }

    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
};


using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;


class OrderModify
{
    public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        : orderId_{ orderId }
        , side_{ side }
        , price_{ price }
        , quantity_{ quantity }
    { }

    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    Quantity GetQuantity() const {return quantity_; } 

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    }

    private:
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity quantity_;
};


struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    // Side side_;
    Quantity quantity_;
};



class Trade
{
    public:
        Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
            : bidTrade_{ bidTrade }
            , askTrade_{ askTrade }
        { }

        const TradeInfo& GetBidTrade() const { return bidTrade_; }
        const TradeInfo& GetAskTrade() const { return askTrade_; }
    
    private:
        TradeInfo bidTrade_;
        TradeInfo askTrade_;
};


using Trades = std::vector<Trade>;



class OrderBook
{
    private:
        
        struct OrderEntry
        {
            OrderPointer order_{ nullptr };
            OrderPointers::iterator location_;
        };

        std::map<Price, OrderPointers, std::greater<int>> bids_;
        std::map<Price, OrderPointers, std::less<int>> asks_;
        std::unordered_map<OrderId, OrderEntry> orders_;
    
        bool CanMatch(Side side, int price) const 
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
        }

        Trades MatchOrders()
        {
            Trades trades;
            trades.reserve(bids_.size());

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

    public:
        
        Trades AddOrder(OrderPointer order)
        {
            if (orders_.contains(order->GetOrderId())) {
                return { };
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
        

        void CancelOrder(OrderId orderId) {

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
};



int main(){
    // lil testing
    OrderBook orderbook;
    const OrderId orderId = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    return 0;
}
