#pragma once

#include <fastbook/itch/messages.h>
#include <fastbook/order_book.h>
#include <fastbook/types.h>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace fastbook::itch {
inline static std::optional<Side> to_side(char c) {
    switch (c) {
        case 'B':
            return Side::BUY;
        case 'S':
            return Side::SELL;
        default:
            return std::nullopt;
    }
}

class OrderBookBuilder {
public:
    void on_add_order(const AddOrder& order) {
        auto side = to_side(order.side);
        if (side.has_value()) {
            order_books_[order.header.stock_locate].add(order.order_reference_number, side.value(),
                                                        order.price, order.shares);
        }
    }

    void on_add_order_with_mpid(const AddOrderWithMPID& order) {
        auto side = to_side(order.side);
        if (side.has_value()) {
            order_books_[order.header.stock_locate].add(order.order_reference_number, side.value(),
                                                        order.price, order.shares);
        }
    }

    void on_execute_order(const ExecuteOrder& order) {
        order_books_[order.header.stock_locate].execute(order.order_reference_number, order.shares);
    }

    void on_execute_order_with_price(const ExecuteOrderWithPrice& order) {
        order_books_[order.header.stock_locate].execute(order.order_reference_number, order.shares);
    }

    void on_cancel_order(const CancelOrder& order) {
        order_books_[order.header.stock_locate].cancel(order.order_reference_number, order.shares);
    }

    void on_delete_order(const DeleteOrder& order) {
        order_books_[order.header.stock_locate].remove(order.order_reference_number);
    }

    void on_replace_order(const ReplaceOrder& order) {
        order_books_[order.header.stock_locate].replace(order.original_order_reference_number,
                                                        order.new_order_reference_number,
                                                        order.shares, order.price);
    }

    const OrderBook& book(std::uint16_t stock_locate) const { return order_books_[stock_locate]; }

    const std::size_t bas_side() const {
        return bad_side_;
    }
private:
    static constexpr std::size_t kLocates = std::numeric_limits<std::uint16_t>::max() + 1;
    std::vector<OrderBook> order_books_ = std::vector<OrderBook>(kLocates);
    std::size_t bad_side_ = 0;
};
}  // namespace fastbook::itch
