#include <vector>
#include <exception>
#include <iostream>
#include <memory>
#include <map>
#include <unordered_map>
#include <list>
#include <format>
#include <numeric>


using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
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
// Organises price-level information for both sides of the order book.
{
    private: 
        LevelInfos bids_;
        LevelInfos asks_;

    public:
        OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
            : bids_{ bids }
            , asks_{ asks }
        { }

        const LevelInfos& GetBids() const { return bids_; }
        const LevelInfos& GetAsks() const { return asks_; }
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
        if (quantity > GetRemainingQuantity()) 
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than remaining quantity", GetOrderId()));
        
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

// NOTE: we use a list for O(1) cancels
using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;


class OrderModify
// Represents updates to an existing order
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

        std::map<Price, OrderPointers, std::greater<Price>> bids_;
        std::map<Price, OrderPointers, std::less<Price>> asks_;
        std::unordered_map<OrderId, OrderEntry> orders_;
    
        bool CanMatch(Side side, Price price) const 
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

        Trades MatchOrders()
        // Returns a list of trades that are generated by insertion of a new order
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

    public:
        
        Trades AddOrder(OrderPointer order)
        {
            if (orders_.contains(order->GetOrderId())) {
                std::cout<< "Rejecting Order : " << order->GetOrderId() << ". Already present in Order Book." << std::endl;
                return { };
            }
        
            if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice())) {
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

        Trades ModifyOrder(OrderModify order) {
            if (!orders_.contains(order.GetOrderId())) {
                return { };
            }
            
            const auto& [existing_order, _] = orders_.at(order.GetOrderId());
            CancelOrder(existing_order->GetOrderId());
            return AddOrder(order.ToOrderPointer(existing_order->GetOrderType()));
        }

        std::size_t Size() const {return orders_.size();}

        OrderBookLevelInfos GetOrderInfos() const {
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
};


int main()
{
    // lil testing
    OrderBook orderbook;
    const OrderId orderId = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    std::cout<< orderbook.Size() << std::endl;
    orderbook.CancelOrder(1);
    std::cout<< orderbook.Size() << std::endl;
    std::cout<<"Hello World" << std::endl;
    return 0;
}
