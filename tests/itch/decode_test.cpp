// decode.h goes first on purpose: if it is not self-contained, this file does not compile.
#include <fastbook/itch/decode.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace itch = fastbook::itch;

namespace {

// Writes `value` big-endian into n bytes of buf, starting at buf[offset].
void put(std::uint8_t* buf, std::size_t offset, std::uint64_t value, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        buf[offset + i] = static_cast<std::uint8_t>(value >> (8 * (n - 1 - i)));
    }
}

// Writes text (not byte-swapped) into buf[offset ...].
void put_text(std::uint8_t* buf, std::size_t offset, const std::string& text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        buf[offset + i] = static_cast<std::uint8_t>(text[i]);
    }
}

// Fields common to every message, at the offsets from the ITCH specification
// (the type letter is at offset 0). Every byte value is different, so a swapped or
// shifted field cannot pass by accident.
void put_header(std::uint8_t* m, char type) {
    m[0] = static_cast<std::uint8_t>(type);
    put(m, 1, 0x0102, 2);             // stock locate
    put(m, 3, 0x0304, 2);             // tracking number
    put(m, 5, 0x05060708090Aull, 6);  // timestamp (6 bytes)
}

void expect_header(const itch::Header& h) {
    EXPECT_EQ(h.stock_locate, 0x0102u);
    EXPECT_EQ(h.tracking_number, 0x0304u);
    EXPECT_EQ(h.timestamp, 0x05060708090Aull);
}

}  // namespace

// decode_* take a pointer to the message WITHOUT the type byte, so every test builds the
// message as in the specification table and passes `m + 1`.

TEST(ItchDecodeTest, Header) {
    std::uint8_t m[itch::AddOrder::length] = {};
    put_header(m, 'A');
    itch::Header h{};
    itch::decode_header(m + 1, h);
    expect_header(h);
}

TEST(ItchDecodeTest, AddOrder) {
    std::uint8_t m[itch::AddOrder::length] = {};
    put_header(m, 'A');
    put(m, 11, 0x1112131415161718ull, 8);  // order reference number
    m[19] = 'B';                           // buy/sell
    put(m, 20, 0x21222324, 4);             // shares
    put_text(m, 24, "AAPL    ");           // stock
    put(m, 32, 0x31323334, 4);             // price

    const itch::AddOrder msg = itch::decode_add_order(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.order_reference_number, 0x1112131415161718ull);
    EXPECT_EQ(msg.side, 'B');
    EXPECT_EQ(msg.shares, 0x21222324u);
    EXPECT_EQ(std::string(msg.stock, 8), "AAPL    ");
    EXPECT_EQ(msg.price, 0x31323334u);
}

TEST(ItchDecodeTest, AddOrderWithMPID) {
    std::uint8_t m[itch::AddOrderWithMPID::length] = {};
    put_header(m, 'F');
    put(m, 11, 0x1112131415161718ull, 8);
    m[19] = 'S';
    put(m, 20, 0x21222324, 4);
    put_text(m, 24, "MSFT    ");
    put(m, 32, 0x31323334, 4);
    put_text(m, 36, "NSDQ");  // attribution

    const itch::AddOrderWithMPID msg = itch::decode_add_order_with_mpid(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.order_reference_number, 0x1112131415161718ull);
    EXPECT_EQ(msg.side, 'S');
    EXPECT_EQ(msg.shares, 0x21222324u);
    EXPECT_EQ(std::string(msg.stock, 8), "MSFT    ");
    EXPECT_EQ(msg.price, 0x31323334u);
    EXPECT_EQ(std::string(msg.mpid, 4), "NSDQ");
}

TEST(ItchDecodeTest, ExecuteOrder) {
    std::uint8_t m[itch::ExecuteOrder::length] = {};
    put_header(m, 'E');
    put(m, 11, 0x1112131415161718ull, 8);  // order reference number
    put(m, 19, 0x21222324, 4);             // executed shares
    put(m, 23, 0x4142434445464748ull, 8);  // match number

    const itch::ExecuteOrder msg = itch::decode_execute_order(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.order_reference_number, 0x1112131415161718ull);
    EXPECT_EQ(msg.shares, 0x21222324u);
    EXPECT_EQ(msg.match_number, 0x4142434445464748ull);
}

TEST(ItchDecodeTest, ExecuteOrderWithPrice) {
    std::uint8_t m[itch::ExecuteOrderWithPrice::length] = {};
    put_header(m, 'C');
    put(m, 11, 0x1112131415161718ull, 8);
    put(m, 19, 0x21222324, 4);
    put(m, 23, 0x4142434445464748ull, 8);
    m[31] = 'Y';                // printable
    put(m, 32, 0x31323334, 4);  // execution price

    const itch::ExecuteOrderWithPrice msg = itch::decode_execute_order_with_price(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.order_reference_number, 0x1112131415161718ull);
    EXPECT_EQ(msg.shares, 0x21222324u);
    EXPECT_EQ(msg.match_number, 0x4142434445464748ull);
    EXPECT_EQ(msg.printable, 'Y');
    EXPECT_EQ(msg.price, 0x31323334u);
}

TEST(ItchDecodeTest, CancelOrder) {
    std::uint8_t m[itch::CancelOrder::length] = {};
    put_header(m, 'X');
    put(m, 11, 0x1112131415161718ull, 8);
    put(m, 19, 0x21222324, 4);  // cancelled shares

    const itch::CancelOrder msg = itch::decode_cancel_order(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.order_reference_number, 0x1112131415161718ull);
    EXPECT_EQ(msg.shares, 0x21222324u);
}

TEST(ItchDecodeTest, DeleteOrder) {
    std::uint8_t m[itch::DeleteOrder::length] = {};
    put_header(m, 'D');
    put(m, 11, 0x1112131415161718ull, 8);

    const itch::DeleteOrder msg = itch::decode_delete_order(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.order_reference_number, 0x1112131415161718ull);
}

TEST(ItchDecodeTest, ReplaceOrder) {
    std::uint8_t m[itch::ReplaceOrder::length] = {};
    put_header(m, 'U');
    put(m, 11, 0x1112131415161718ull, 8);  // original order reference number
    put(m, 19, 0x5152535455565758ull, 8);  // new order reference number
    put(m, 27, 0x21222324, 4);             // shares
    put(m, 31, 0x31323334, 4);             // price

    const itch::ReplaceOrder msg = itch::decode_replace_order(m + 1);
    expect_header(msg.header);
    EXPECT_EQ(msg.original_order_reference_number, 0x1112131415161718ull);
    EXPECT_EQ(msg.new_order_reference_number, 0x5152535455565758ull);
    EXPECT_EQ(msg.shares, 0x21222324u);
    EXPECT_EQ(msg.price, 0x31323334u);
}
