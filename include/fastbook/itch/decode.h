#pragma once

#include <fastbook/itch/messages.h>
#include <fastbook/utils/byte_order.h>
#include <cstdint>
#include <cstring>

namespace fastbook::itch {
namespace utils = fastbook::utils;

/*
About guarantees: Caller must guarantee at least T::length readable bytes at ptr,
and that ptr[0] is the message type. No bounds checking is done here
*/

/*
Decoders for the ITCH 5.0 messages that change the order book: A, F, E, C, X, D, U
(plus decode_header). The other 16 message types in messages.h are intentionally not
decoded yet: they do not affect the book and the parser will skip them by length.
They can be added later, one function and one test each.
*/

inline void decode_header(const std::uint8_t* ptr, Header& header) {
    header.stock_locate = utils::read_uint16_be(ptr + 1);
    header.tracking_number = utils::read_uint16_be(ptr + 3);
    header.timestamp = utils::read_uint48_be(ptr + 5);
}

inline AddOrder decode_add_order(const std::uint8_t* ptr) {
    AddOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = utils::read_uint64_be(ptr + 11);
    message.side = *(ptr + 19);
    message.shares = utils::read_uint32_be(ptr + 20);
    std::memcpy(message.stock, ptr + 24, sizeof message.stock);
    message.price = utils::read_uint32_be(ptr + 32);
    return message;
}

inline AddOrderWithMPID decode_add_order_with_mpid(const std::uint8_t* ptr) {
    AddOrderWithMPID message;
    decode_header(ptr, message.header);
    message.order_reference_number = utils::read_uint64_be(ptr + 11);
    message.side = *(ptr + 19);
    message.shares = utils::read_uint32_be(ptr + 20);
    std::memcpy(message.stock, ptr + 24, sizeof message.stock);
    message.price = utils::read_uint32_be(ptr + 32);
    std::memcpy(message.mpid, ptr + 36, sizeof message.mpid);
    return message;
}

inline ExecuteOrder decode_execute_order(const std::uint8_t* ptr) {
    ExecuteOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = utils::read_uint64_be(ptr + 11);
    message.shares = utils::read_uint32_be(ptr + 19);
    message.match_number = utils::read_uint64_be(ptr + 23);
    return message;
}

inline ExecuteOrderWithPrice decode_execute_order_with_price(const std::uint8_t* ptr) {
    ExecuteOrderWithPrice message;
    decode_header(ptr, message.header);
    message.order_reference_number = utils::read_uint64_be(ptr + 11);
    message.shares = utils::read_uint32_be(ptr + 19);
    message.match_number = utils::read_uint64_be(ptr + 23);
    message.printable = *(ptr + 31);
    message.price = utils::read_uint32_be(ptr + 32);
    return message;
}

inline CancelOrder decode_cancel_order(const std::uint8_t* ptr) {
    CancelOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = utils::read_uint64_be(ptr + 11);
    message.shares = utils::read_uint32_be(ptr + 19);
    return message;
}

inline DeleteOrder decode_delete_order(const std::uint8_t* ptr) {
    DeleteOrder message;
    decode_header(ptr, message.header);
    message.order_reference_number = utils::read_uint64_be(ptr + 11);
    return message;
}

inline ReplaceOrder decode_replace_order(const std::uint8_t* ptr) {
    ReplaceOrder message;
    decode_header(ptr, message.header);
    message.original_order_reference_number = utils::read_uint64_be(ptr + 11);
    message.new_order_reference_number = utils::read_uint64_be(ptr + 19);
    message.shares = utils::read_uint32_be(ptr + 27);
    message.price = utils::read_uint32_be(ptr + 31);
    return message;
}
}  // namespace fastbook::itch
