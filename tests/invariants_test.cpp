#include <gtest/gtest.h>

#include <fastbook/order_book.h>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

#include "reference_book.h"

using namespace fastbook;
using fastbook::test::ReferenceBook;

namespace {

constexpr Price kMinPrice = 95;
constexpr Price kPriceSpan = 11;
constexpr unsigned kMaxQuantity = 40;

struct Driver {
    explicit Driver(unsigned seed) : seed_(seed), rng_(seed) {}

    OrderBook book;
    ReferenceBook ref;

    void step(int index) {
        step_ = index;
        const unsigned dice = rng_() % 100;

        if (dice < 35) {
            op_ = "add";
            const OrderID id = (rng_() % 20 == 0) ? pick_id() : next_id_++;
            const Side side = (rng_() & 1) ? Side::BUY : Side::SELL;
            const Price price = kMinPrice + rng_() % kPriceSpan;
            const Quantity qty = rng_() % (kMaxQuantity + 1);
            ASSERT_EQ(book.add(id, side, price, qty), ref.add(id, side, price, qty)) << ctx();
        } else if (dice < 55) {
            op_ = "execute";
            const OrderID id = pick_id();
            const Quantity qty = rng_() % (kMaxQuantity + 1);
            ASSERT_EQ(book.execute(id, qty), ref.reduce(id, qty)) << ctx();
        } else if (dice < 70) {
            op_ = "cancel";
            const OrderID id = pick_id();
            const Quantity qty = rng_() % (kMaxQuantity + 1);
            ASSERT_EQ(book.cancel(id, qty), ref.reduce(id, qty)) << ctx();
        } else if (dice < 80) {
            op_ = "remove";
            const OrderID id = pick_id();
            ASSERT_EQ(book.remove(id), ref.remove(id)) << ctx();
        } else {
            op_ = "replace";
            const OrderID old_id = pick_id();
            const OrderID new_id = (rng_() % 20 == 0) ? pick_id() : next_id_++;
            const Quantity qty = rng_() % (kMaxQuantity + 1);
            const Price price = kMinPrice + rng_() % kPriceSpan;
            ASSERT_EQ(book.replace(old_id, new_id, qty, price),
                      ref.replace(old_id, new_id, qty, price))
                << ctx();
        }

        ASSERT_TRUE(book.check_invariants()) << ctx();
        compare_state();
    }

private:
    OrderID pick_id() {
        const std::vector<OrderID> live = ref.live_ids();
        if (!live.empty() && rng_() % 10 != 0) return live[rng_() % live.size()];
        return 1 + rng_() % (next_id_ + 2);
    }

    std::string ctx() const {
        return "seed=" + std::to_string(seed_) + " step=" + std::to_string(step_) + " op=" + op_;
    }

    void compare_state() {
        for (Side side : {Side::BUY, Side::SELL}) {
            for (unsigned n : {1u, 3u, 100u}) {
                const auto got = book.depth(side, n);
                const auto want = ref.depth(side, n);
                ASSERT_EQ(got.size(), want.size()) << ctx();
                for (std::size_t k = 0; k < got.size(); ++k) {
                    ASSERT_EQ(got[k].price, want[k].price) << ctx();
                    ASSERT_EQ(got[k].total_quantity, want[k].total_quantity) << ctx();
                    ASSERT_EQ(got[k].orderIds.size(), want[k].queue.size()) << ctx();
                    std::size_t j = 0;
                    for (const auto& order : got[k].orderIds) {
                        ASSERT_EQ(order.orderId, want[k].queue[j].first) << ctx();
                        ASSERT_EQ(order.quantity, want[k].queue[j].second) << ctx();
                        ++j;
                    }
                }
            }
        }

        const auto bid = book.best_bid();
        const auto want_bid = ref.depth(Side::BUY, 1);
        ASSERT_EQ(bid.has_value(), !want_bid.empty()) << ctx();
        if (bid) {
            ASSERT_EQ(bid->price, want_bid[0].price) << ctx();
            ASSERT_EQ(bid->total_quantity, want_bid[0].total_quantity) << ctx();
        }

        const auto ask = book.best_ask();
        const auto want_ask = ref.depth(Side::SELL, 1);
        ASSERT_EQ(ask.has_value(), !want_ask.empty()) << ctx();
        if (ask) {
            ASSERT_EQ(ask->price, want_ask[0].price) << ctx();
            ASSERT_EQ(ask->total_quantity, want_ask[0].total_quantity) << ctx();
        }
    }

    unsigned seed_;
    std::mt19937 rng_;
    OrderID next_id_ = 1;
    int step_ = 0;
    const char* op_ = "";
};

}  // namespace

TEST(InvariantsTest, EmptyBookIsValid) {
    OrderBook book;
    EXPECT_TRUE(book.check_invariants());
}

TEST(InvariantsTest, InvariantsHoldAfterEachSimpleOperation) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    ASSERT_TRUE(book.check_invariants());
    book.add(2, Side::BUY, 100, 5);
    ASSERT_TRUE(book.check_invariants());
    book.add(3, Side::SELL, 101, 7);
    ASSERT_TRUE(book.check_invariants());
    book.execute(1, 4);
    ASSERT_TRUE(book.check_invariants());
    book.cancel(2, 5);
    ASSERT_TRUE(book.check_invariants());
    book.replace(3, 4, 9, 102);
    ASSERT_TRUE(book.check_invariants());
    book.remove(1);
    ASSERT_TRUE(book.check_invariants());
    book.remove(4);
    ASSERT_TRUE(book.check_invariants());
}

TEST(InvariantsTest, RandomOperationsMatchReferenceAndKeepInvariants) {
    for (unsigned seed = 1; seed <= 20; ++seed) {
        Driver driver(seed);
        for (int i = 0; i < 3000; ++i) {
            ASSERT_NO_FATAL_FAILURE(driver.step(i));
        }
    }
}

TEST(InvariantsTest, RemovingAllOrdersLeavesEmptyBook) {
    Driver driver(42);
    for (int i = 0; i < 3000; ++i) {
        ASSERT_NO_FATAL_FAILURE(driver.step(i));
    }

    for (OrderID id : driver.ref.live_ids()) {
        ASSERT_TRUE(driver.book.remove(id));
        ASSERT_TRUE(driver.ref.remove(id));
        ASSERT_TRUE(driver.book.check_invariants());
    }

    EXPECT_FALSE(driver.book.best_bid().has_value());
    EXPECT_FALSE(driver.book.best_ask().has_value());
    EXPECT_TRUE(driver.book.depth(Side::BUY, 10).empty());
    EXPECT_TRUE(driver.book.depth(Side::SELL, 10).empty());
    EXPECT_TRUE(driver.book.check_invariants());
}
