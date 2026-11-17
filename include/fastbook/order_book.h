#pragma once

#include <optional>
#include <cstdint>
#include <fastbook/types.h>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <list>
#include <array>

namespace fastbook {
struct Order {
    OrderID orderId;
    Quantity quantity;
};

struct Level {
    Quantity total_quantity;
    Price price;
    std::list<Order> orderIds;
};

struct OrderLoc {
    Side side;
    Price price;
    std::list<Order>::iterator orderIt;
};

enum ErrorCode {
    ORDER_ID_EXISTS,
    ORDER_ID_NOT_FOUND,
    INSUFFICIENT_QUANTITY,
    INVALID_QUANTITY,
    COUNT // always last
};

class OrderBook {
public:
    bool add(OrderID orderId, Side side, Price price, Quantity quantity);
    bool execute(OrderID orderId, Quantity quantity);
    bool cancel(OrderID orderId, Quantity quantity);
    bool remove(OrderID orderId);
    bool replace(OrderID oldOrderId, OrderID newOrderId, Quantity quantity, Price price);

    std::optional<Level> best_bid() const;
    std::optional<Level> best_ask() const;
    std::vector<Level> depth(Side side, uint32_t n) const;

    uint32_t error_count(ErrorCode code) const;
    bool check_invariants() const;
private:
    bool reduce_quantity(OrderID orderId, Quantity quantity);

    std::map<Price, Level, std::greater<Price>> bids_;
    std::map<Price, Level> asks_;
    std::unordered_map<OrderID, OrderLoc> index_;
    std::array<uint32_t, COUNT> error_codes_{};
};
} // namespace fastbook