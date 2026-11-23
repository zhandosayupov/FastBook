#pragma once

#include <fastbook/utils/byte_order.h>


namespace fastbook::itch {
struct ParseStats {
    std::size_t consumed = 0;
    std::size_t messages = 0;
    std::size_t skipped = 0;
    std::size_t bad_length = 0;
};

template <typename Handler>
ParseStats parse(const std::uint8_t* data, std::size_t size, Handler& handler) { // TODO: how to define scheme of Handler? maybe virtual class interface? or some asserts? What are trade-offs? 
    ParseStats stats;
    std::size_t pos = 0;
    while (size - pos >= 2) {
        std::size_t msg_size = utils::read_uint16_be(data + pos);
        pos += 2;
        char msg_type = *(data + pos);
        // TODO how to differ bad sizes? 
        switch(msg_type) {
            case 'A':
                if (size - pos >= msg_size) {
                    handler.on_add_order(decode_add_order(data + pos));
                }
                break;
            case 'F':
                if (size - pos >= msg_size) {
                    handler.on_add_order_with_mpid(decode_add_order_with_mpid(data + pos));
                }
                break;
            case 'E':
                if (size - pos >= msg_size) {
                    handler.on_execute_order(decode_execute_order(data + pos));
                }
                break;
            case 'C':
                if (size - pos >= msg_size) {
                    handler.on_execute_order_with_price(decode_execute_order_with_price(data + pos));
                }
                break;
            case 'X':
                if (size - pos >= msg_size) {
                    handler.on_cancel_order(decode_cancel_order(data + pos));
                }
                break;
            case 'D':
                if (size - pos >= msg_size) {
                    handler.on_delete_order(decode_delete_order(data + pos));
                }
                break;
            case 'U':
                if (size - pos >= msg_size) {
                    handler.on_replace_order(decode_replace_order(data + pos));
                }
                break;
            default:
                stats.messages++;
        }
        pos += msg_size;
        stats.consumed += msg_size;
    }
}

} // namespace fastbook::itch
