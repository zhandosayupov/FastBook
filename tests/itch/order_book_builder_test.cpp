#include <fastbook/itch/order_book_builder.h>

#include <fastbook/itch/parser.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace itch = fastbook::itch;
using fastbook::ErrorCode;

namespace {

// Messages are built as structs: decoding is covered by decode_test.cpp, here only the
// mapping "message -> order book call" matters. Prices and quantities are always different
// numbers, so a swapped argument cannot pass.

constexpr std::uint16_t kLocate = 7;

itch::AddOrder add(std::uint64_t ref, char side, std::uint32_t shares, std::uint32_t price,
                   std::uint16_t locate = kLocate) {
    itch::AddOrder m{};
    m.header.stock_locate = locate;
    m.order_reference_number = ref;
    m.side = side;
    m.shares = shares;
    m.price = price;
    return m;
}

itch::AddOrderWithMPID add_mpid(std::uint64_t ref, char side, std::uint32_t shares,
                                std::uint32_t price, std::uint16_t locate = kLocate) {
    itch::AddOrderWithMPID m{};
    m.header.stock_locate = locate;
    m.order_reference_number = ref;
    m.side = side;
    m.shares = shares;
    m.price = price;
    return m;
}

itch::ExecuteOrder execute(std::uint64_t ref, std::uint32_t shares,
                           std::uint16_t locate = kLocate) {
    itch::ExecuteOrder m{};
    m.header.stock_locate = locate;
    m.order_reference_number = ref;
    m.shares = shares;
    return m;
}

itch::ExecuteOrderWithPrice execute_with_price(std::uint64_t ref, std::uint32_t shares,
                                               std::uint32_t price,
                                               std::uint16_t locate = kLocate) {
    itch::ExecuteOrderWithPrice m{};
    m.header.stock_locate = locate;
    m.order_reference_number = ref;
    m.shares = shares;
    m.printable = 'Y';
    m.price = price;
    return m;
}

itch::CancelOrder cancel(std::uint64_t ref, std::uint32_t shares, std::uint16_t locate = kLocate) {
    itch::CancelOrder m{};
    m.header.stock_locate = locate;
    m.order_reference_number = ref;
    m.shares = shares;
    return m;
}

itch::DeleteOrder del(std::uint64_t ref, std::uint16_t locate = kLocate) {
    itch::DeleteOrder m{};
    m.header.stock_locate = locate;
    m.order_reference_number = ref;
    return m;
}

itch::ReplaceOrder replace(std::uint64_t old_ref, std::uint64_t new_ref, std::uint32_t shares,
                           std::uint32_t price, std::uint16_t locate = kLocate) {
    itch::ReplaceOrder m{};
    m.header.stock_locate = locate;
    m.original_order_reference_number = old_ref;
    m.new_order_reference_number = new_ref;
    m.shares = shares;
    m.price = price;
    return m;
}

}  // namespace

static_assert(itch::ItchHandler<itch::OrderBookBuilder>);

TEST(OrderBookBuilderTest, AddGoesToTheRightSide) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'B', 10, 1000));
    b.on_add_order(add(2, 'S', 20, 1100));

    const auto bid = b.book(kLocate).best_bid();
    const auto ask = b.book(kLocate).best_ask();
    ASSERT_TRUE(bid);
    ASSERT_TRUE(ask);
    EXPECT_EQ(bid->price, 1000u);
    EXPECT_EQ(bid->total_quantity, 10u);
    EXPECT_EQ(ask->price, 1100u);
    EXPECT_EQ(ask->total_quantity, 20u);
}

TEST(OrderBookBuilderTest, AddWithMpidWorksLikeAdd) {
    itch::OrderBookBuilder b;
    b.on_add_order_with_mpid(add_mpid(1, 'S', 20, 1100));

    const auto ask = b.book(kLocate).best_ask();
    ASSERT_TRUE(ask);
    EXPECT_EQ(ask->price, 1100u);
    EXPECT_EQ(ask->total_quantity, 20u);
    EXPECT_FALSE(b.book(kLocate).best_bid());
}

// A side byte other than 'B' / 'S' is rejected and the book is not touched.
TEST(OrderBookBuilderTest, UnknownSideIsIgnored) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'Z', 10, 1000));
    b.on_add_order_with_mpid(add_mpid(2, 'b', 10, 1000));

    EXPECT_FALSE(b.book(kLocate).best_bid());
    EXPECT_FALSE(b.book(kLocate).best_ask());
}

// E and C carry the executed shares (to subtract), not the remaining size.
TEST(OrderBookBuilderTest, ExecuteSubtractsExecutedShares) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'B', 100, 1000));
    b.on_execute_order(execute(1, 30));
    b.on_execute_order_with_price(execute_with_price(1, 20, 999));

    const auto bid = b.book(kLocate).best_bid();
    ASSERT_TRUE(bid);
    EXPECT_EQ(bid->price, 1000u);  // execution price of C does not move the order
    EXPECT_EQ(bid->total_quantity, 50u);
}

TEST(OrderBookBuilderTest, FullExecutionRemovesOrder) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'S', 40, 1100));
    b.on_execute_order(execute(1, 40));

    EXPECT_FALSE(b.book(kLocate).best_ask());
}

TEST(OrderBookBuilderTest, CancelSubtractsCancelledShares) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'S', 100, 1100));
    b.on_cancel_order(cancel(1, 30));

    const auto ask = b.book(kLocate).best_ask();
    ASSERT_TRUE(ask);
    EXPECT_EQ(ask->total_quantity, 70u);
}

TEST(OrderBookBuilderTest, DeleteRemovesOrder) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'B', 10, 1000));
    b.on_add_order(add(2, 'B', 20, 900));
    b.on_delete_order(del(1));

    const auto bid = b.book(kLocate).best_bid();
    ASSERT_TRUE(bid);
    EXPECT_EQ(bid->price, 900u);
    EXPECT_EQ(bid->total_quantity, 20u);
}

// U has no side: the new order keeps the side of the old one, gets the new id, shares and
// price. Shares and price differ, so passing them to replace() in the wrong order fails.
TEST(OrderBookBuilderTest, ReplaceKeepsSideAndUsesNewIdSharesPrice) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'S', 10, 1100));
    b.on_replace_order(replace(1, 2, 7, 1050));

    const auto ask = b.book(kLocate).best_ask();
    ASSERT_TRUE(ask);
    EXPECT_FALSE(b.book(kLocate).best_bid());
    EXPECT_EQ(ask->price, 1050u);
    EXPECT_EQ(ask->total_quantity, 7u);
    ASSERT_EQ(ask->orderIds.size(), 1u);
    EXPECT_EQ(ask->orderIds.front().orderId, 2u);

    // The old id is gone, the new one is live.
    b.on_delete_order(del(1));
    EXPECT_EQ(b.book(kLocate).error_count(ErrorCode::ORDER_ID_NOT_FOUND), 1u);
    b.on_delete_order(del(2));
    EXPECT_FALSE(b.book(kLocate).best_ask());
}

// Each stock locate has its own book: messages for one locate never touch another.
TEST(OrderBookBuilderTest, LocatesAreIndependent) {
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'B', 10, 1000, 1));
    b.on_add_order(add(2, 'B', 20, 2000, 2));
    b.on_execute_order(execute(1, 4, 1));
    b.on_delete_order(del(2, 2));

    const auto bid1 = b.book(1).best_bid();
    ASSERT_TRUE(bid1);
    EXPECT_EQ(bid1->price, 1000u);
    EXPECT_EQ(bid1->total_quantity, 6u);
    EXPECT_FALSE(b.book(2).best_bid());

    // A message routed to the wrong locate does not find the order there.
    b.on_delete_order(del(1, 2));
    EXPECT_EQ(b.book(2).error_count(ErrorCode::ORDER_ID_NOT_FOUND), 1u);
    EXPECT_TRUE(b.book(1).best_bid());
}

// The largest locate (65535) has a book too: books_ must hold 65536 entries. The ASan
// build reports a heap overflow if it is one short.
TEST(OrderBookBuilderTest, LargestLocateHasABook) {
    constexpr std::uint16_t kMax = 65535;
    itch::OrderBookBuilder b;
    b.on_add_order(add(1, 'S', 10, 1100, kMax));

    const auto ask = b.book(kMax).best_ask();
    ASSERT_TRUE(ask);
    EXPECT_EQ(ask->price, 1100u);
}

// End to end: raw ITCH bytes -> parse -> builder -> book.
TEST(OrderBookBuilderTest, ParseIntoBuilder) {
    // Writes `value` big-endian into n bytes at s[at].
    auto put = [](std::vector<std::uint8_t>& s, std::size_t at, std::uint64_t value,
                  std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) {
            s[at + i] = static_cast<std::uint8_t>(value >> (8 * (n - 1 - i)));
        }
    };
    // Appends a frame (2-byte length + message with type and stock locate) and returns the
    // offset of the message.
    auto frame = [&](std::vector<std::uint8_t>& s, char type, std::size_t len) {
        const std::size_t at = s.size() + 2;
        s.resize(at + len, 0);
        put(s, at - 2, len, 2);
        s[at] = static_cast<std::uint8_t>(type);
        put(s, at + 1, kLocate, 2);
        return at;
    };

    std::vector<std::uint8_t> s;
    std::size_t m = frame(s, 'A', itch::AddOrder::length);
    put(s, m + 11, 1, 8);     // order ref
    s[m + 19] = 'B';          // side
    put(s, m + 20, 10, 4);    // shares
    put(s, m + 32, 1000, 4);  // price

    m = frame(s, 'A', itch::AddOrder::length);
    put(s, m + 11, 2, 8);
    s[m + 19] = 'S';
    put(s, m + 20, 20, 4);
    put(s, m + 32, 1100, 4);

    m = frame(s, 'E', itch::ExecuteOrder::length);
    put(s, m + 11, 2, 8);  // order ref
    put(s, m + 19, 5, 4);  // executed shares

    m = frame(s, 'D', itch::DeleteOrder::length);
    put(s, m + 11, 1, 8);

    itch::OrderBookBuilder b;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), b);

    EXPECT_EQ(st.consumed, s.size());
    EXPECT_EQ(st.messages, 4u);
    EXPECT_FALSE(b.book(kLocate).best_bid());
    const auto ask = b.book(kLocate).best_ask();
    ASSERT_TRUE(ask);
    EXPECT_EQ(ask->price, 1100u);
    EXPECT_EQ(ask->total_quantity, 15u);
}
