#pragma once

#include <fastbook/itch/messages.h>
#include <fastbook/types.h>
#include <fastbook/order_book.h>
#include <optional> 

namespace fastbook::itch {
    inline static std::optional<Side> to_side(char c) {
        switch (c) {
            case 'B': return Side::BUY;
            case 'S': return Side::SELL;
            default:  return std::nullopt;
        }
    }

    class OrderBookBuilder {
    public:
        OrderBookBuilder ();        void on_add_order(const AddOrder& order) {
            auto side = to_side(order.side);
            if (side.has_value()) {
                order_book_.add(order.order_reference_number, side.value(), order.price, order.shares);
            }
        }

        void on_add_order_with_mpid(const AddOrderWithMPID& order) {
            auto side = to_side(order.side);
            if (side.has_value()) {
                order_book_.add(order.order_reference_number, side.value(), order.price, order.shares);
            }
        }

        void on_execute_order(const ExecuteOrder& order) {
            order_book_.execute(order.order_reference_number, order.shares);
        }

        void on_execute_order_with_price(const ExecuteOrderWithPrice& order) {
            order_book_.execute(order.order_reference_number, order.shares);
        }

        void on_cancel_order(const CancelOrder& order) {
            order_book_.cancel(order.order_reference_number, order.shares);
        }
        
        void on_delete_order(const DeleteOrder& order) {
            order_book_.remove(order.order_reference_number);
        }

        void on_replace_order(const ReplaceOrder& order) {
            order_book_.replace(order.original_order_reference, order.new_order_reference_number, order.shares, order.price);
        }

    private:
        OrderBook order_book_;
    };
}  // namespace fastbook::itch
