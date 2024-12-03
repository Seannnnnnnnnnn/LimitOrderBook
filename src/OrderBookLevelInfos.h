#pragma once

#include "LevelInfo.h"


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