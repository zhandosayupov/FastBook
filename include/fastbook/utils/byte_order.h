#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace fastbook::utils {
inline std::uint32_t read_uint32_be(const std::uint8_t* data) {
    return (static_cast<std::uint32_t>(data[0]) << 24) |
           (static_cast<std::uint32_t>(data[1]) << 16) |
           (static_cast<std::uint32_t>(data[2]) << 8) | (static_cast<std::uint32_t>(data[3]));
}

inline std::uint64_t read_uint64_be(const std::uint8_t* data) {
    return (static_cast<std::uint64_t>(data[0]) << 56) |
           (static_cast<std::uint64_t>(data[1]) << 48) |
           (static_cast<std::uint64_t>(data[2]) << 40) |
           (static_cast<std::uint64_t>(data[3]) << 32) |
           (static_cast<std::uint64_t>(data[4]) << 24) |
           (static_cast<std::uint64_t>(data[5]) << 16) |
           (static_cast<std::uint64_t>(data[6]) << 8) | (static_cast<std::uint64_t>(data[7]));
}

inline std::uint16_t read_uint16_be(const std::uint8_t* data) {
    return (static_cast<std::uint16_t>(data[0]) << 8) | (static_cast<std::uint16_t>(data[1]));
}

inline std::uint64_t read_uint48_be(const std::uint8_t* data) {
    return (static_cast<std::uint64_t>(data[0]) << 40) |
           (static_cast<std::uint64_t>(data[1]) << 32) |
           (static_cast<std::uint64_t>(data[2]) << 24) |
           (static_cast<std::uint64_t>(data[3]) << 16) |
           (static_cast<std::uint64_t>(data[4]) << 8) | (static_cast<std::uint64_t>(data[5]));
}
}  // namespace fastbook::utils