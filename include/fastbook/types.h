#pragma once

#include <cstdint>


namespace fastbook {
// type for Price, Quantity: uint32_t
// type for OrderID: uint64_t
using Price = uint32_t;
using Quantity = uint32_t;
using OrderID = uint64_t;

enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};
} // namespace fastbook