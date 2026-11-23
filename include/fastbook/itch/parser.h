#pragma once

#include <fastbook/itch/decode.h>
#include <fastbook/itch/messages.h>
#include <fastbook/utils/byte_order.h>
#include <cstddef>
#include <cstdint>

namespace fastbook::itch {
struct ParseStats {
    std::size_t consumed = 0;
    std::size_t messages = 0;
    std::size_t skipped = 0;
    std::size_t bad_length = 0;
};

template <typename Handler>
ParseStats parse(const std::uint8_t* data, std::size_t size,
                 Handler& handler) {  // TODO: how to define scheme of Handler? maybe virtual class
                                      // interface? or some asserts? What are trade-offs?
    ParseStats stats;
    std::size_t pos = 0;
    while (size - pos >= 2) {
        std::size_t len = fastbook::utils::read_uint16_be(data + pos);
        char msg_type = *(data + pos);
        pos += 2;
        switch (msg_type) {
            case AddOrder::type:
                if (len == AddOrder::length) {
                    handler.on_add_order(decode_add_order(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case AddOrderWithMPID::type:
                if (len == AddOrderWithMPID::length) {
                    handler.on_add_order_with_mpid(decode_add_order_with_mpid(data + pos));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case ExecuteOrder::type:
                if (len == ExecuteOrder::length) {
                    handler.on_execute_order(decode_execute_order(data + pos));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case ExecuteOrderWithPrice::type:
                if (len == ExecuteOrderWithPrice::length) {
                    handler.on_execute_order_with_price(
                        decode_execute_order_with_price(data + pos));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case CancelOrder::type:
                if (len == CancelOrder::length) {
                    handler.on_cancel_order(decode_cancel_order(data + pos));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case DeleteOrder::type:
                if (len == DeleteOrder::length) {
                    handler.on_delete_order(decode_delete_order(data + pos));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case ReplaceOrder::type:
                if (len == ReplaceOrder::length) {
                    handler.on_replace_order(decode_replace_order(data + pos));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            default:
                ++stats.skipped;
            pos += len;
        }
        stats.consumed = pos;
        return stats;
    }
}

}  // namespace fastbook::itch
