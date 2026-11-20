#pragma once

#include <cstring>
#include <cstdint>
#include <fastbook/itch/messages.h>
#include <fastbook/utils/byte_order.h>

using namespace fastbook::utils;


namespace fastbook::itch {
/*
Submit message ptr without message type identifier
*/
inline void decode_header(std::uint8_t* ptr, Header& header) {
    header.stock_locate = read_uint16_be(ptr);
    header.tracking_number = read_uint16_be(ptr + 2);
    header.timestamp = read_uint48_be(ptr + 4);
}

inline AddOrder decode_add_order(std::uint8_t* ptr) {
    AddOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = read_uint64_be(ptr + 10);
    message.side = *(ptr + 18);
    message.shares = read_uint32_be(ptr + 19);
    std::memcpy(message.stock, ptr + 23, sizeof message.stock);
    message.price = read_uint32_be(ptr + 31);
    return message;
}


inline AddOrderWithMPID decode_add_order_with_mpid(std::uint8_t* ptr) {
    AddOrderWithMPID message;
    decode_header(ptr, message.header);
    message.order_reference_number = read_uint64_be(ptr + 10);
    message.side = *(ptr + 18);
    message.shares = read_uint32_be(ptr + 19);
    std::memcpy(message.stock, ptr + 23, sizeof message.stock);
    message.price = read_uint32_be(ptr + 31);
    std::memcpy(message.mpid, ptr + 35, sizeof message.mpid);
    return message;
}

inline ExecuteOrder decode_execute_order(std::uint8_t* ptr) {
    ExecuteOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = read_uint64_be(ptr + 10);
    message.shares = read_uint32_be(ptr + 18);
    message.match_number = read_uint64_be(ptr + 22);
    return message;
}

inline ExecuteOrderWithPrice decode_execute_order_with_price(std::uint8_t* ptr) {
    ExecuteOrderWithPrice message;
    decode_header(ptr, message.header);
    message.order_reference_number = read_uint64_be(ptr + 10);
    message.shares = read_uint32_be(ptr + 18);
    message.match_number = read_uint64_be(ptr + 22);
    message.printable = *(ptr + 30);
    message.price = read_uint32_be(ptr + 31);
    return message;
}

inline CancelOrder decode_cancel_order(std::uint8_t* ptr) {
    CancelOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = read_uint64_be(ptr + 10);
    message.shares = read_uint32_be(ptr + 18);
    return message;
}


inline DeleteOrder decode_delete_order(std::uint8_t* ptr) {
    DeleteOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = read_uint64_be(ptr + 10);
    return message;
}

inline ReplaceOrder decode_replace_order(std::uint8_t* ptr) {
    ReplaceOrder message;
    decode_header(ptr, message.header);
    message.original_order_reference_number = read_uint64_be(ptr + 10);
    message.new_order_reference_number = read_uint64_be(ptr + 18);
    message.shares = read_uint32_be(ptr + 26);
    message.price = read_uint32_be(ptr + 30);
    return message;
}
} // namespace fastbook::itch

