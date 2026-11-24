#include <fastbook/order_book.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace fastbook;

namespace {

// Full observable state of the book: every level of both sides with its queue
// (id and quantity of each order, in queue order).
using QueueSnapshot = std::vector<std::pair<OrderID, Quantity>>;
using LevelSnapshot = std::tuple<Side, Price, Quantity, QueueSnapshot>;

std::vector<LevelSnapshot> snapshot(const OrderBook& book) {
    std::vector<LevelSnapshot> out;
    for (Side side : {Side::BUY, Side::SELL}) {
        for (const Level& level : book.depth(side, 1000)) {
            QueueSnapshot queue;
            for (const Order& o : level.orderIds) queue.emplace_back(o.orderId, o.quantity);
            out.emplace_back(side, level.price, level.total_quantity, queue);
        }
    }
    return out;
}

std::vector<std::uint32_t> counters(const OrderBook& book) {
    std::vector<std::uint32_t> out;
    for (std::size_t c = 0; c < ErrorCode::COUNT; ++c) {
        out.push_back(book.error_count(static_cast<ErrorCode>(c)));
    }
    return out;
}

// Two bid levels (two orders at 100, one at 99) and one ask level.
OrderBook make_book() {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 5);
    book.add(3, Side::BUY, 99, 7);
    book.add(4, Side::SELL, 101, 8);
    return book;
}

struct FailingOp {
    std::string name;
    std::function<bool(OrderBook&)> op;
    ErrorCode expected;
};

}  // namespace

// Every rejected operation: returns false, increments exactly its own counter by one,
// leaves the other counters at 0, and does not change the book.
TEST(ErrorCountTest, RejectedOperationCountsOnceAndLeavesBookUnchanged) {
    const std::vector<FailingOp> cases = {
        {"execute unknown id", [](OrderBook& b) { return b.execute(42, 1); }, ORDER_ID_NOT_FOUND},
        {"cancel unknown id", [](OrderBook& b) { return b.cancel(42, 1); }, ORDER_ID_NOT_FOUND},
        {"remove unknown id", [](OrderBook& b) { return b.remove(42); }, ORDER_ID_NOT_FOUND},
        {"replace unknown old id", [](OrderBook& b) { return b.replace(42, 9, 5, 100); },
         ORDER_ID_NOT_FOUND},
        {"add duplicate id", [](OrderBook& b) { return b.add(1, Side::SELL, 105, 3); },
         ORDER_ID_EXISTS},
        {"replace to taken new id", [](OrderBook& b) { return b.replace(1, 4, 5, 100); },
         ORDER_ID_EXISTS},
        {"execute more than left", [](OrderBook& b) { return b.execute(2, 6); },
         INSUFFICIENT_QUANTITY},
        {"cancel more than left", [](OrderBook& b) { return b.cancel(3, 8); },
         INSUFFICIENT_QUANTITY},
        {"add zero quantity", [](OrderBook& b) { return b.add(9, Side::BUY, 100, 0); },
         INVALID_QUANTITY},
        {"replace with zero quantity", [](OrderBook& b) { return b.replace(1, 9, 0, 100); },
         INVALID_QUANTITY},
    };

    for (const FailingOp& c : cases) {
        SCOPED_TRACE(c.name);
        OrderBook book = make_book();
        const auto before = snapshot(book);

        EXPECT_FALSE(c.op(book));

        std::vector<std::uint32_t> expected(ErrorCode::COUNT, 0);
        expected[c.expected] = 1;
        EXPECT_EQ(counters(book), expected);
        EXPECT_EQ(snapshot(book), before);
        EXPECT_TRUE(book.check_invariants());
    }
}

// A valid sequence of every operation leaves all counters at 0: a non-zero counter on a
// correct feed means a bug.
TEST(ErrorCountTest, ValidOperationsLeaveAllCountersAtZero) {
    OrderBook book = make_book();
    EXPECT_TRUE(book.execute(1, 4));  // partial
    EXPECT_TRUE(book.cancel(2, 5));   // full: removes the order
    EXPECT_TRUE(book.replace(3, 5, 2, 98));
    EXPECT_TRUE(book.execute(4, 8));  // full: removes the ask level
    EXPECT_TRUE(book.remove(1));
    EXPECT_TRUE(book.remove(5));

    EXPECT_EQ(counters(book), std::vector<std::uint32_t>(ErrorCode::COUNT, 0));
    EXPECT_TRUE(snapshot(book).empty());
    EXPECT_TRUE(book.check_invariants());
}

// Counters accumulate per kind and do not reset.
TEST(ErrorCountTest, CountersAccumulate) {
    OrderBook book = make_book();
    book.remove(42);
    book.execute(43, 1);
    book.add(1, Side::BUY, 100, 1);

    EXPECT_EQ(book.error_count(ORDER_ID_NOT_FOUND), 2u);
    EXPECT_EQ(book.error_count(ORDER_ID_EXISTS), 1u);
    EXPECT_EQ(book.error_count(INSUFFICIENT_QUANTITY), 0u);
    EXPECT_EQ(book.error_count(INVALID_QUANTITY), 0u);
}
