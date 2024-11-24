#include <memory>
#include <list>
#include <vector>

using Price = int;
using Quantity = uint32_t;
using OrderId = uint32_t;
using OrderIds = std::vector<OrderId>;
using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

