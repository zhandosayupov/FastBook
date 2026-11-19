#pragma once

// Decoded Nasdaq TotalView-ITCH 5.0 messages.
//
// Source: "Nasdaq TotalView-ITCH 5.0" specification (36 pages), section 1 "Message Formats".
// These structs hold already decoded values, they are NOT laid over the raw bytes:
// the wire format has no padding and is big-endian, so fields are read one by one at
// the offsets given in the comments (see utils/byte_order.h).
//
// Conventions
//  - `type`   is the message type letter (byte 0 of the message).
//  - `length` is the total message length in bytes, without the 2-byte length prefix.
//  - Comments are "offset, length in bytes" of the field in the message.
//  - Alpha fields are ASCII, left justified and right padded with spaces:
//    one character is `char`, several characters are `char[N]`.
//  - Price(4) is a 32-bit integer with 4 implied decimal places, Price(8) a 64-bit one.
//  - Timestamp is nanoseconds since midnight, 6 bytes on the wire, `uint64_t` here.

#include <cstddef>
#include <cstdint>

namespace fastbook::itch {

// Fields at offsets 1..10, common to every message (offset 0 is the message type).
struct Header {
    std::uint16_t stock_locate;     // 1, 2: locate code identifying the security (0 if none)
    std::uint16_t tracking_number;  // 3, 2: Nasdaq internal tracking number
    std::uint64_t timestamp;        // 5, 6: nanoseconds since midnight
};

// ---------------------------------------------------------------------------
// 1.1 System event
// ---------------------------------------------------------------------------

struct SystemEvent {
    static constexpr char type = 'S';
    static constexpr std::size_t length = 12;

    Header header;
    char event_code;  // 11, 1: 'O' start of messages, 'S' start of system hours,
                      //        'Q' start of market hours, 'M' end of market hours,
                      //        'E' end of system hours, 'C' end of messages
};

// ---------------------------------------------------------------------------
// 1.2 Stock related messages
// ---------------------------------------------------------------------------

struct StockDirectory {
    static constexpr char type = 'R';
    static constexpr std::size_t length = 39;

    Header header;
    char stock[8];                                          // 11, 8: stock symbol
    char market_category;                                   // 19, 1: listing market or tier
    char financial_status_indicator;                        // 20, 1
    std::uint32_t round_lot_size;                           // 21, 4
    char round_lots_only;                                   // 25, 1: 'Y' or 'N'
    char issue_classification;                              // 26, 1
    char issue_sub_type[2];                                 // 27, 2
    char authenticity;                                      // 29, 1: 'P' live, 'T' test
    char short_sale_threshold_indicator;                    // 30, 1: 'Y', 'N' or ' '
    char ipo_flag;                                          // 31, 1: 'Y', 'N' or ' '
    char limit_up_limit_down_reference_price_tier;          // 32, 1: '1', '2' or ' '
    char exchange_traded_product_flag;                      // 33, 1: 'Y', 'N' or ' '
    std::uint32_t exchange_traded_product_leverage_factor;  // 34, 4
    char inverse_indicator;                                 // 38, 1: 'Y' or 'N'
};

struct StockTradingAction {
    static constexpr char type = 'H';
    static constexpr std::size_t length = 25;

    Header header;
    char stock[8];       // 11, 8: stock symbol
    char trading_state;  // 19, 1: 'H' halted, 'P' paused, 'Q' quotation only, 'T' trading
    char reserved;       // 20, 1: reserved
    char reason[4];      // 21, 4: trading action reason (appendix C of the spec)
};

struct RegShoShortSalePriceTestRestrictedIndicator {
    static constexpr char type = 'Y';
    static constexpr std::size_t length = 20;

    Header header;
    char stock[8];        // 11, 8: stock symbol
    char reg_sho_action;  // 19, 1: '0' no price test, '1' in effect (intraday drop),
                          //        '2' remains in effect
};

struct MarketParticipantPosition {
    static constexpr char type = 'L';
    static constexpr std::size_t length = 26;

    Header header;
    char mpid[4];                   // 11, 4: market participant identifier
    char stock[8];                  // 15, 8: stock symbol
    char primary_market_maker;      // 23, 1: 'Y' or 'N'
    char market_maker_mode;         // 24, 1: 'N' normal, 'P' passive, 'S' syndicate,
                                    //        'R' pre-syndicate, 'L' penalty
    char market_participant_state;  // 25, 1: 'A' active, 'E' excused/withdrawn,
                                    //        'W' withdrawn, 'S' suspended, 'D' deleted
};

struct MarketWideCircuitBreakerDeclineLevel {
    static constexpr char type = 'V';
    static constexpr std::size_t length = 35;

    Header header;         // stock_locate is always 0
    std::uint64_t level1;  // 11, 8: Price(8)
    std::uint64_t level2;  // 19, 8: Price(8)
    std::uint64_t level3;  // 27, 8: Price(8)
};

struct MarketWideCircuitBreakerStatus {
    static constexpr char type = 'W';
    static constexpr std::size_t length = 12;

    Header header;        // stock_locate is always 0
    char breached_level;  // 11, 1: '1', '2' or '3'
};

struct QuotingPeriodUpdate {
    static constexpr char type = 'K';
    static constexpr std::size_t length = 28;

    Header header;                             // stock_locate is always 0
    char stock[8];                             // 11, 8: stock symbol
    std::uint32_t ipo_quotation_release_time;  // 19, 4: seconds since midnight
    char ipo_quotation_release_qualifier;      // 23, 1: 'A' anticipated, 'C' canceled/postponed
    std::uint32_t ipo_price;                   // 24, 4: Price(4)
};

struct LimitUpLimitDownAuctionCollar {
    static constexpr char type = 'J';
    static constexpr std::size_t length = 35;

    Header header;
    char stock[8];                                 // 11, 8: stock symbol
    std::uint32_t auction_collar_reference_price;  // 19, 4: Price(4)
    std::uint32_t upper_auction_collar_price;      // 23, 4: Price(4)
    std::uint32_t lower_auction_collar_price;      // 27, 4: Price(4)
    std::uint32_t auction_collar_extension;        // 31, 4: number of extensions
};

struct OperationalHalt {
    static constexpr char type = 'h';
    static constexpr std::size_t length = 21;

    Header header;
    char stock[8];                 // 11, 8: stock symbol
    char market_code;              // 19, 1: 'Q' Nasdaq, 'B' BX, 'X' PSX
    char operational_halt_action;  // 20, 1: 'H' halted, 'T' halt lifted
};

// ---------------------------------------------------------------------------
// 1.3 Add order
// ---------------------------------------------------------------------------

struct AddOrder {
    static constexpr char type = 'A';
    static constexpr std::size_t length = 36;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8: unique reference of the new order
    char side;                             // 19, 1: 'B' buy, 'S' sell
    std::uint32_t shares;                  // 20, 4: shares added to the book
    char stock[8];                         // 24, 8: stock symbol
    std::uint32_t price;                   // 32, 4: Price(4), display price
};

struct AddOrderWithMPID {
    static constexpr char type = 'F';
    static constexpr std::size_t length = 40;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8
    char side;                             // 19, 1: 'B' or 'S'
    std::uint32_t shares;                  // 20, 4
    char stock[8];                         // 24, 8: stock symbol
    std::uint32_t price;                   // 32, 4: Price(4)
    char mpid[4];                          // 36, 4: attribution (market participant identifier)
};

// ---------------------------------------------------------------------------
// 1.4 Modify order messages
// ---------------------------------------------------------------------------

struct ExecuteOrder {
    static constexpr char type = 'E';
    static constexpr std::size_t length = 31;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8: order that was executed
    std::uint32_t shares;                  // 19, 4: executed shares
    std::uint64_t match_number;            // 23, 8: day unique match number
};

struct ExecuteOrderWithPrice {
    static constexpr char type = 'C';
    static constexpr std::size_t length = 36;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8
    std::uint32_t shares;                  // 19, 4: executed shares
    std::uint64_t match_number;            // 23, 8
    char printable;                        // 31, 1: 'Y' printable, 'N' non-printable
    std::uint32_t price;                   // 32, 4: Price(4), execution price
};

struct CancelOrder {
    static constexpr char type = 'X';
    static constexpr std::size_t length = 23;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8
    std::uint32_t shares;                  // 19, 4: shares removed from the display size
};

struct DeleteOrder {
    static constexpr char type = 'D';
    static constexpr std::size_t length = 19;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8: order removed from the book
};

struct ReplaceOrder {
    static constexpr char type = 'U';
    static constexpr std::size_t length = 35;

    Header header;
    std::uint64_t original_order_reference_number;  // 11, 8: order being replaced
    std::uint64_t new_order_reference_number;       // 19, 8: used for all later updates
    std::uint32_t shares;                           // 27, 4: new total displayed quantity
    std::uint32_t price;                            // 31, 4: Price(4), new display price
};

// ---------------------------------------------------------------------------
// 1.5 Trade messages
// ---------------------------------------------------------------------------

// Trade (non-cross). Does not change the order book.
struct Trade {
    static constexpr char type = 'P';
    static constexpr std::size_t length = 44;

    Header header;
    std::uint64_t order_reference_number;  // 11, 8: always 0 in the binary feed
    char side;                             // 19, 1: always 'B' since 2014-07-14
    std::uint32_t shares;                  // 20, 4
    char stock[8];                         // 24, 8: stock symbol
    std::uint32_t price;                   // 32, 4: Price(4)
    std::uint64_t match_number;            // 36, 8
};

struct CrossTrade {
    static constexpr char type = 'Q';
    static constexpr std::size_t length = 40;

    Header header;
    std::uint64_t shares;        // 11, 8: shares matched in the cross
    char stock[8];               // 19, 8: stock symbol
    std::uint32_t cross_price;   // 27, 4: Price(4)
    std::uint64_t match_number;  // 31, 8
    char cross_type;             // 39, 1: 'O' opening, 'C' closing, 'H' IPO / halted / paused
};

struct BrokenTrade {
    static constexpr char type = 'B';
    static constexpr std::size_t length = 19;

    Header header;
    std::uint64_t match_number;  // 11, 8: match number of the execution that was broken
};

// ---------------------------------------------------------------------------
// 1.6 Net order imbalance indicator (NOII)
// ---------------------------------------------------------------------------

struct NetOrderImbalanceIndicator {
    static constexpr char type = 'I';
    static constexpr std::size_t length = 50;

    Header header;
    std::uint64_t paired_shares;            // 11, 8
    std::uint64_t imbalance_shares;         // 19, 8
    char imbalance_direction;               // 27, 1: 'B', 'S', 'N', 'O' or 'P'
    char stock[8];                          // 28, 8: stock symbol
    std::uint32_t far_price;                // 36, 4: Price(4)
    std::uint32_t near_price;               // 40, 4: Price(4)
    std::uint32_t current_reference_price;  // 44, 4: Price(4)
    char cross_type;                        // 48, 1: 'O', 'C', 'H' or 'A'
    char price_variation_indicator;         // 49, 1: 'L', '1'..'9', 'A'..'C' or ' '
};

// ---------------------------------------------------------------------------
// 1.7 Retail price improvement indicator (RPII)
// ---------------------------------------------------------------------------

struct RetailPriceImprovementIndicator {
    static constexpr char type = 'N';
    static constexpr std::size_t length = 20;

    Header header;
    char stock[8];       // 11, 8: stock symbol
    char interest_flag;  // 19, 1: 'B' buy, 'S' sell, 'A' both sides, 'N' none
};

// ---------------------------------------------------------------------------
// 1.8 Direct listing with capital raise price discovery
// ---------------------------------------------------------------------------

struct DirectListingWithCapitalRaisePriceDiscovery {
    static constexpr char type = 'O';
    static constexpr std::size_t length = 48;

    Header header;
    char stock[8];                           // 11, 8: stock symbol
    char open_eligibility_status;            // 19, 1: 'Y' eligible, 'N' not eligible
    std::uint32_t minimum_allowable_price;   // 20, 4: Price(4)
    std::uint32_t maximum_allowable_price;   // 24, 4: Price(4)
    std::uint32_t near_execution_price;      // 28, 4: Price(4)
    std::uint64_t near_execution_time;       // 32, 8
    std::uint32_t lower_price_range_collar;  // 40, 4: Price(4)
    std::uint32_t upper_price_range_collar;  // 44, 4: Price(4)
};

}  // namespace fastbook::itch
