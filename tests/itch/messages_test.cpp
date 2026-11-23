#include <fastbook/itch/messages.h>
#include <gtest/gtest.h>

#include <cstddef>

namespace itch = fastbook::itch;

namespace {

// Type letter and message length (without the 2-byte length prefix), written by hand from
// the ITCH 5.0 specification. A typo in messages.h breaks the build here.
template <typename T>
constexpr bool spec(char type, std::size_t length) {
    return T::type == type && T::length == length;
}

static_assert(spec<itch::SystemEvent>('S', 12));
static_assert(spec<itch::StockDirectory>('R', 39));
static_assert(spec<itch::StockTradingAction>('H', 25));
static_assert(spec<itch::RegShoShortSalePriceTestRestrictedIndicator>('Y', 20));
static_assert(spec<itch::MarketParticipantPosition>('L', 26));
static_assert(spec<itch::MarketWideCircuitBreakerDeclineLevel>('V', 35));
static_assert(spec<itch::MarketWideCircuitBreakerStatus>('W', 12));
static_assert(spec<itch::QuotingPeriodUpdate>('K', 28));
static_assert(spec<itch::LimitUpLimitDownAuctionCollar>('J', 35));
static_assert(spec<itch::OperationalHalt>('h', 21));
static_assert(spec<itch::AddOrder>('A', 36));
static_assert(spec<itch::AddOrderWithMPID>('F', 40));
static_assert(spec<itch::ExecuteOrder>('E', 31));
static_assert(spec<itch::ExecuteOrderWithPrice>('C', 36));
static_assert(spec<itch::CancelOrder>('X', 23));
static_assert(spec<itch::DeleteOrder>('D', 19));
static_assert(spec<itch::ReplaceOrder>('U', 35));
static_assert(spec<itch::Trade>('P', 44));
static_assert(spec<itch::CrossTrade>('Q', 40));
static_assert(spec<itch::BrokenTrade>('B', 19));
static_assert(spec<itch::NetOrderImbalanceIndicator>('I', 50));
static_assert(spec<itch::RetailPriceImprovementIndicator>('N', 20));
static_assert(spec<itch::DirectListingWithCapitalRaisePriceDiscovery>('O', 48));

}  // namespace

TEST(ItchMessagesTest, AddOrderLength) { EXPECT_EQ(itch::AddOrder::length, 36u); }
