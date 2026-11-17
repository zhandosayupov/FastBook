#include <gtest/gtest.h>
#include <fastbook/order_book.h>

#include <vector>

using namespace fastbook;

namespace {
// Порядок id заявок в очереди уровня (голова очереди первая).
std::vector<OrderID> queue_ids(const Level& level) {
    std::vector<OrderID> ids;
    for (const auto& o : level.orderIds) ids.push_back(o.orderId);
    return ids;
}
}  // namespace

// ---------- базовые операции ----------

TEST(OrderBookTest, AddOrder) {
    OrderBook book;
    EXPECT_TRUE(book.add(1, Side::BUY, 100, 10));
    EXPECT_FALSE(book.add(1, Side::BUY, 100, 10));  // дубликат id
}

TEST(OrderBookTest, ExecuteOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_TRUE(book.execute(1, 5));
    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 5u);

    EXPECT_FALSE(book.execute(1, 10));  // больше остатка
    EXPECT_EQ(book.best_bid()->total_quantity, 5u);  // книга не изменилась
}

TEST(OrderBookTest, CancelOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_TRUE(book.cancel(1, 5));
    EXPECT_EQ(book.best_bid()->total_quantity, 5u);

    EXPECT_FALSE(book.cancel(1, 10));  // больше остатка
    EXPECT_EQ(book.best_bid()->total_quantity, 5u);
}

TEST(OrderBookTest, RemoveOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_TRUE(book.remove(1));
    EXPECT_FALSE(book.best_bid().has_value());  // уровень исчез
    EXPECT_FALSE(book.remove(1));               // уже нет
}

TEST(OrderBookTest, UnknownIdIsRejected) {
    OrderBook book;
    EXPECT_FALSE(book.execute(42, 1));
    EXPECT_FALSE(book.cancel(42, 1));
    EXPECT_FALSE(book.remove(42));
}

// ---------- best / depth ----------

TEST(OrderBookTest, BestBidAndAsk) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::SELL, 101, 5);

    auto bid = book.best_bid();
    auto ask = book.best_ask();
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(bid->price, 100u);
    EXPECT_EQ(bid->total_quantity, 10u);
    EXPECT_EQ(ask->price, 101u);
    EXPECT_EQ(ask->total_quantity, 5u);
}

TEST(OrderBookTest, BestBidIsHighestAndBestAskIsLowest) {
    OrderBook book;
    book.add(1, Side::BUY, 99, 1);
    book.add(2, Side::BUY, 100, 1);
    book.add(3, Side::BUY, 98, 1);
    book.add(4, Side::SELL, 103, 1);
    book.add(5, Side::SELL, 101, 1);
    book.add(6, Side::SELL, 102, 1);

    EXPECT_EQ(book.best_bid()->price, 100u);
    EXPECT_EQ(book.best_ask()->price, 101u);
}

TEST(OrderBookTest, BestBidAndAskWithNoOrders) {
    OrderBook book;
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(OrderBookTest, TwoOrdersOnSameLevelSumQuantity) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 4);

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 14u);
    EXPECT_EQ(bid->orderIds.size(), 2u);
}

TEST(OrderBookTest, Depth) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 99, 5);
    book.add(3, Side::SELL, 101, 5);
    book.add(4, Side::SELL, 102, 10);

    auto bids = book.depth(Side::BUY, 2);
    auto asks = book.depth(Side::SELL, 2);

    ASSERT_EQ(bids.size(), 2u);
    ASSERT_EQ(asks.size(), 2u);

    EXPECT_EQ(bids[0].price, 100u);
    EXPECT_EQ(bids[0].total_quantity, 10u);
    EXPECT_EQ(bids[1].price, 99u);
    EXPECT_EQ(bids[1].total_quantity, 5u);

    EXPECT_EQ(asks[0].price, 101u);
    EXPECT_EQ(asks[0].total_quantity, 5u);
    EXPECT_EQ(asks[1].price, 102u);
    EXPECT_EQ(asks[1].total_quantity, 10u);
}

TEST(OrderBookTest, DepthWithFewerLevelsThanRequested) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::SELL, 101, 5);

    auto bids = book.depth(Side::BUY, 5);
    auto asks = book.depth(Side::SELL, 5);

    ASSERT_EQ(bids.size(), 1u);
    ASSERT_EQ(asks.size(), 1u);
    EXPECT_EQ(bids[0].price, 100u);
    EXPECT_EQ(bids[0].total_quantity, 10u);
    EXPECT_EQ(asks[0].price, 101u);
    EXPECT_EQ(asks[0].total_quantity, 5u);
}

TEST(OrderBookTest, DepthWithNoOrders) {
    OrderBook book;
    EXPECT_TRUE(book.depth(Side::BUY, 5).empty());
    EXPECT_TRUE(book.depth(Side::SELL, 5).empty());
}

// ---------- уровни исчезают, best сдвигается ----------

TEST(OrderBookTest, ExecutingLastOrderOnLevelShiftsBest) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 99, 5);

    ASSERT_TRUE(book.execute(1, 10));  // полностью

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 99u);
    EXPECT_EQ(book.depth(Side::BUY, 5).size(), 1u);
}

// ---------- приоритет внутри уровня (FIFO) ----------

TEST(OrderBookTest, AddAppendsToQueueTail) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 5);
    book.add(2, Side::BUY, 100, 5);
    book.add(3, Side::BUY, 100, 5);

    EXPECT_EQ(queue_ids(*book.best_bid()), (std::vector<OrderID>{1, 2, 3}));
}

TEST(OrderBookTest, FullExecuteOfFirstOrderRemovesItFromHead) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 5);
    book.add(2, Side::BUY, 100, 5);
    book.add(3, Side::BUY, 100, 5);

    ASSERT_TRUE(book.execute(1, 5));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 10u);
    EXPECT_EQ(queue_ids(*bid), (std::vector<OrderID>{2, 3}));
}

TEST(OrderBookTest, PartialExecuteKeepsOrderAtHead) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 5);

    ASSERT_TRUE(book.execute(1, 4));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 11u);
    EXPECT_EQ(queue_ids(*bid), (std::vector<OrderID>{1, 2}));
    EXPECT_EQ(bid->orderIds.front().quantity, 6u);
}

TEST(OrderBookTest, ExecuteTargetsGivenOrderNotHeadOfQueue) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 5);

    ASSERT_TRUE(book.execute(2, 5));  // исполняется именно заявка 2

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 10u);
    EXPECT_EQ(queue_ids(*bid), (std::vector<OrderID>{1}));
    EXPECT_EQ(bid->orderIds.front().quantity, 10u);  // заявка 1 не тронута
}

TEST(OrderBookTest, PartialCancelKeepsQueuePosition) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 5);
    book.add(3, Side::BUY, 100, 5);

    ASSERT_TRUE(book.cancel(2, 2));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 18u);
    EXPECT_EQ(queue_ids(*bid), (std::vector<OrderID>{1, 2, 3}));  // место сохранено
    auto it = std::next(bid->orderIds.begin());
    EXPECT_EQ(it->quantity, 3u);
}

TEST(OrderBookTest, FullCancelRemovesOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 5);

    ASSERT_TRUE(book.cancel(1, 10));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 5u);
    EXPECT_EQ(queue_ids(*bid), (std::vector<OrderID>{2}));
}

TEST(OrderBookTest, RemoveMiddleOrderKeepsOthersInOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 1);
    book.add(2, Side::BUY, 100, 2);
    book.add(3, Side::BUY, 100, 3);

    ASSERT_TRUE(book.remove(2));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 4u);
    EXPECT_EQ(queue_ids(*bid), (std::vector<OrderID>{1, 3}));
}

// ---------- replace ----------

TEST(OrderBookTest, ReplaceOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_TRUE(book.replace(1, 2, 5, 101));   // (old, new, qty, price)
    EXPECT_FALSE(book.replace(1, 3, 10, 102)); // старой заявки уже нет
}

TEST(OrderBookTest, ReplaceWithLargerQuantityIsAllowed) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_TRUE(book.replace(1, 2, 500, 101));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 101u);
    EXPECT_EQ(bid->total_quantity, 500u);
}

TEST(OrderBookTest, ReplaceWithExistingNewIdMustFailAndKeepOldOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 99, 5);

    EXPECT_FALSE(book.replace(1, 2, 7, 101));  // id 2 уже занят

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100u);
    EXPECT_EQ(bid->total_quantity, 10u);
    EXPECT_TRUE(book.remove(1));  // заявка 1 всё ещё существует
}

TEST(OrderBookTest, ReplaceWithZeroQuantityKeepsOldOrder) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_FALSE(book.replace(1, 2, 0, 101));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100u);
    EXPECT_EQ(bid->total_quantity, 10u);
}

TEST(OrderBookTest, ReplaceUnknownOldIdChangesNothing) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    EXPECT_FALSE(book.replace(99, 2, 5, 101));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100u);
    EXPECT_EQ(bid->total_quantity, 10u);
}

TEST(OrderBookTest, ReplaceKeepsSideOfOldOrder) {
    OrderBook book;
    book.add(1, Side::SELL, 105, 10);

    ASSERT_TRUE(book.replace(1, 2, 8, 106));

    EXPECT_FALSE(book.best_bid().has_value());
    auto ask = book.best_ask();
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(ask->price, 106u);
    EXPECT_EQ(ask->total_quantity, 8u);
}

TEST(OrderBookTest, ReplaceSamePriceUpdatesLevelQuantity) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);
    book.add(2, Side::BUY, 100, 4);

    ASSERT_TRUE(book.replace(1, 3, 6, 100));

    auto bid = book.best_bid();
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->total_quantity, 10u);  // 4 + 6
}

TEST(OrderBookTest, ReplaceLosesQueuePriority) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 5);
    book.add(2, Side::BUY, 100, 5);
    book.add(3, Side::BUY, 100, 5);

    ASSERT_TRUE(book.replace(1, 4, 5, 100));  // та же цена, новая заявка встаёт в хвост

    EXPECT_EQ(queue_ids(*book.best_bid()), (std::vector<OrderID>{2, 3, 4}));
}

TEST(OrderBookTest, ReplaceToNewPriceRemovesEmptyOldLevel) {
    OrderBook book;
    book.add(1, Side::BUY, 100, 10);

    ASSERT_TRUE(book.replace(1, 2, 10, 95));

    auto bids = book.depth(Side::BUY, 5);
    ASSERT_EQ(bids.size(), 1u);
    EXPECT_EQ(bids[0].price, 95u);
}

// ---------- нулевое количество ----------

TEST(OrderBookTest, AddZeroQuantityIsRejected) {
    OrderBook book;
    EXPECT_FALSE(book.add(1, Side::BUY, 100, 0));
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST(OrderBookTest, ZeroQuantityLevelDoesNotAppearInDepth) {
    OrderBook book;
    book.add(1, Side::SELL, 101, 0);
    EXPECT_TRUE(book.depth(Side::SELL, 5).empty());
}
