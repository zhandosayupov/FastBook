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

template <typename H>
concept ItchHandler = requires(H& h, const AddOrder& a, const AddOrderWithMPID& f,
                               const ExecuteOrder& e, const ExecuteOrderWithPrice& c,
                               const CancelOrder& x, const DeleteOrder& d, const ReplaceOrder& u) {
    h.on_add_order(a);
    h.on_add_order_with_mpid(f);
    h.on_execute_order(e);
    h.on_execute_order_with_price(c);
    h.on_cancel_order(x);
    h.on_delete_order(d);
    h.on_replace_order(u);
};

template <ItchHandler Handler>
ParseStats parse(const std::uint8_t* data, std::size_t size, Handler& handler) {
    ParseStats stats;
    std::size_t pos = 0;
    while (size - pos >= 2) {
        std::size_t len = fastbook::utils::read_uint16_be(data + pos);
        if (size - pos - 2 < len) {
            break;
        }
        const std::uint8_t* msg = data + pos + 2;
        if (len == 0) {
            ++stats.bad_length;
        }
        char msg_type = *(msg);
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
                    handler.on_add_order_with_mpid(decode_add_order_with_mpid(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case ExecuteOrder::type:
                if (len == ExecuteOrder::length) {
                    handler.on_execute_order(decode_execute_order(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case ExecuteOrderWithPrice::type:
                if (len == ExecuteOrderWithPrice::length) {
                    handler.on_execute_order_with_price(decode_execute_order_with_price(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case CancelOrder::type:
                if (len == CancelOrder::length) {
                    handler.on_cancel_order(decode_cancel_order(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case DeleteOrder::type:
                if (len == DeleteOrder::length) {
                    handler.on_delete_order(decode_delete_order(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            case ReplaceOrder::type:
                if (len == ReplaceOrder::length) {
                    handler.on_replace_order(decode_replace_order(msg));
                    ++stats.messages;
                } else {
                    ++stats.bad_length;
                }
                break;
            default:
                if (len) {
                    ++stats.skipped;
                }
                break;
        }
        pos += len + 2;
        stats.consumed = pos;
    }
    return stats;
}

}  // namespace fastbook::itch
