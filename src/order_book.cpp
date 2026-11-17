#include <fastbook/order_book.h>

using namespace fastbook;

bool OrderBook::add(OrderID orderId, Side side, Price price, Quantity quantity) {
    if (index_.contains(orderId)) {
        error_codes_[ErrorCode::ORDER_ID_EXISTS]++;
        return false;
    }
    if (quantity == 0) {
        error_codes_[ErrorCode::INVALID_QUANTITY]++;
        return false;
    }
    
    Level& level = (side == Side::BUY) ? bids_[price] : asks_[price];
    level.price = price;
    level.total_quantity += quantity;
    level.orderIds.emplace_back(orderId, quantity);
    index_[orderId] = OrderLoc{side, price, std::prev(level.orderIds.end())};
    return true;
}

bool OrderBook::reduce_quantity(OrderID orderId, Quantity quantity) {
    auto it = index_.find(orderId);
    if (it == index_.end()) {
        error_codes_[ErrorCode::ORDER_ID_NOT_FOUND]++;
        return false;
    }

    OrderLoc& orderLoc = it->second;
    Level& level = (orderLoc.side == Side::BUY) ? bids_[orderLoc.price] : asks_[orderLoc.price];

    if (orderLoc.orderIt->quantity < quantity) {
        error_codes_[ErrorCode::INSUFFICIENT_QUANTITY]++;
        return false;
    }

    orderLoc.orderIt->quantity -= quantity;
    level.total_quantity -= quantity;

    if (orderLoc.orderIt->quantity == 0) {
        level.orderIds.erase(orderLoc.orderIt);
        index_.erase(it);
    }

    if (level.total_quantity == 0) {
        if (orderLoc.side == Side::BUY) {
            bids_.erase(orderLoc.price);
        } else {
            asks_.erase(orderLoc.price);
        }
    }

    return true;
}

bool OrderBook::execute(OrderID orderId, Quantity quantity) {
    return reduce_quantity(orderId, quantity);
}

bool OrderBook::cancel(OrderID orderId, Quantity quantity) {
    return reduce_quantity(orderId, quantity);
}

bool OrderBook::remove(OrderID orderId) {
    auto it = index_.find(orderId);
    if (it == index_.end()) {
        error_codes_[ErrorCode::ORDER_ID_NOT_FOUND]++;
        return false;
    }
    return reduce_quantity(orderId, it->second.orderIt->quantity);
}

bool OrderBook::replace(OrderID oldOrderId, OrderID newOrderId, Quantity quantity, Price price) {
    auto idx = index_.find(oldOrderId);
    if (idx == index_.end()) {
        ++error_codes_[static_cast<size_t>(ErrorCode::ORDER_ID_NOT_FOUND)];
        return false;
    }
    if (index_.contains(newOrderId)) {
        ++error_codes_[static_cast<size_t>(ErrorCode::ORDER_ID_EXISTS)];
        return false;
    }
    if (quantity == 0) {
        ++error_codes_[static_cast<size_t>(ErrorCode::INVALID_QUANTITY)];
        return false;
    }

    Side side = idx->second.side;   
    remove(oldOrderId);
    return add(newOrderId, side, price, quantity);
}

std::optional<Level> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->second;
}

std::optional<Level> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->second;
}

std::vector<Level> OrderBook::depth(Side side, uint32_t n) const {
    std::vector<Level> result;
    if (side == Side::BUY) {
        for (const auto& [price, level] : bids_) {
            if (result.size() >= n) break;
            result.push_back(level);
        }
    } else {
        for (const auto& [price, level] : asks_) {
            if (result.size() >= n) break;
            result.push_back(level);
        }
    }
    return result;
}
