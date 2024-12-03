#pragma once

#include "Usings.h"

struct LevelInfo
{
    Price price_;
    Quantity quantity;
};


using LevelInfos = std::vector<LevelInfo>;