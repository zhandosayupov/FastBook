#include <fastbook/itch/parser.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace itch = fastbook::itch;

namespace {

// One entry per handler call: which method was called and for which order.
struct Call {
    char type;
    std::uint64_t order_reference_number;

    bool operator==(const Call&) const = default;
};

// Test spy: records every call in order. Field decoding is covered by decode_test.cpp,
// so here only "which method, which order, in which sequence" matters.
struct RecordingHandler {
    std::vector<Call> calls;

    void on_add_order(const itch::AddOrder& m) { calls.push_back({'A', m.order_reference_number}); }
    void on_add_order_with_mpid(const itch::AddOrderWithMPID& m) {
        calls.push_back({'F', m.order_reference_number});
    }
    void on_execute_order(const itch::ExecuteOrder& m) {
        calls.push_back({'E', m.order_reference_number});
    }
    void on_execute_order_with_price(const itch::ExecuteOrderWithPrice& m) {
        calls.push_back({'C', m.order_reference_number});
    }
    void on_cancel_order(const itch::CancelOrder& m) {
        calls.push_back({'X', m.order_reference_number});
    }
    void on_delete_order(const itch::DeleteOrder& m) {
        calls.push_back({'D', m.order_reference_number});
    }
    void on_replace_order(const itch::ReplaceOrder& m) {
        calls.push_back({'U', m.original_order_reference_number});
    }
};

static_assert(itch::ItchHandler<RecordingHandler>);

// Appends one frame: 2-byte big-endian length, then `len` message bytes.
// Byte 0 = type; bytes 11..18 = order reference (same offset in A, F, E, C, X, D, U).
// `len` is separate from the type so a test can build a frame with a wrong length.
void append(std::vector<std::uint8_t>& s, char type, std::size_t len, std::uint64_t ref = 0) {
    s.push_back(static_cast<std::uint8_t>(len >> 8));
    s.push_back(static_cast<std::uint8_t>(len));
    const std::size_t start = s.size();
    s.resize(start + len, 0);
    if (len > 0) {
        s[start] = static_cast<std::uint8_t>(type);
    }
    if (len >= 19) {
        for (std::size_t i = 0; i < 8; ++i) {
            s[start + 11 + i] = static_cast<std::uint8_t>(ref >> (8 * (7 - i)));
        }
    }
}

// Size of a whole frame: length prefix + message.
constexpr std::size_t frame(std::size_t len) { return 2 + len; }

}  // namespace

// Every handled type goes to its own method, in stream order.
TEST(ItchParserTest, DispatchesAllSevenTypesInOrder) {
    std::vector<std::uint8_t> s;
    append(s, 'A', itch::AddOrder::length, 1);
    append(s, 'F', itch::AddOrderWithMPID::length, 2);
    append(s, 'E', itch::ExecuteOrder::length, 3);
    append(s, 'C', itch::ExecuteOrderWithPrice::length, 4);
    append(s, 'X', itch::CancelOrder::length, 5);
    append(s, 'D', itch::DeleteOrder::length, 6);
    append(s, 'U', itch::ReplaceOrder::length, 7);

    RecordingHandler h;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), h);

    EXPECT_EQ(st.consumed, s.size());
    EXPECT_EQ(st.messages, 7u);
    EXPECT_EQ(st.skipped, 0u);
    EXPECT_EQ(st.bad_length, 0u);
    const std::vector<Call> expected{{'A', 1}, {'F', 2}, {'E', 3}, {'C', 4},
                                     {'X', 5}, {'D', 6}, {'U', 7}};
    EXPECT_EQ(h.calls, expected);
}

TEST(ItchParserTest, EmptyInput) {
    const std::uint8_t dummy = 0;
    RecordingHandler h;
    const itch::ParseStats st = itch::parse(&dummy, 0, h);

    EXPECT_EQ(st.consumed, 0u);
    EXPECT_EQ(st.messages, 0u);
    EXPECT_TRUE(h.calls.empty());
}

// One byte of the next length prefix: not enough to read the length, stop before it.
TEST(ItchParserTest, TruncatedLengthPrefix) {
    std::vector<std::uint8_t> s;
    append(s, 'D', itch::DeleteOrder::length, 9);
    s.push_back(0x00);

    RecordingHandler h;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), h);

    EXPECT_EQ(st.consumed, frame(itch::DeleteOrder::length));
    EXPECT_EQ(h.calls, (std::vector<Call>{{'D', 9}}));
}

// Length is known but the body is cut: handler not called, consumed stops at the prefix.
TEST(ItchParserTest, TruncatedBody) {
    std::vector<std::uint8_t> s;
    append(s, 'D', itch::DeleteOrder::length, 9);
    append(s, 'A', itch::AddOrder::length, 10);
    s.resize(s.size() - 1);

    RecordingHandler h;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), h);

    EXPECT_EQ(st.consumed, frame(itch::DeleteOrder::length));
    EXPECT_EQ(st.messages, 1u);
    EXPECT_EQ(h.calls, (std::vector<Call>{{'D', 9}}));
}

// A valid ITCH message without a decoder is skipped by its length; the next one is parsed.
TEST(ItchParserTest, UnknownTypeSkipped) {
    std::vector<std::uint8_t> s;
    append(s, 'S', itch::SystemEvent::length);
    append(s, 'D', itch::DeleteOrder::length, 9);

    RecordingHandler h;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), h);

    EXPECT_EQ(st.consumed, s.size());
    EXPECT_EQ(st.skipped, 1u);
    EXPECT_EQ(st.messages, 1u);
    EXPECT_EQ(st.bad_length, 0u);
    EXPECT_EQ(h.calls, (std::vector<Call>{{'D', 9}}));
}

// Known type with a wrong length: not decoded, counted, skipped by its own length.
TEST(ItchParserTest, WrongLengthForKnownType) {
    std::vector<std::uint8_t> s;
    append(s, 'A', itch::AddOrder::length - 1, 10);
    append(s, 'D', itch::DeleteOrder::length, 9);

    RecordingHandler h;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), h);

    EXPECT_EQ(st.consumed, s.size());
    EXPECT_EQ(st.bad_length, 1u);
    EXPECT_EQ(st.messages, 1u);
    EXPECT_EQ(st.skipped, 0u);
    EXPECT_EQ(h.calls, (std::vector<Call>{{'D', 9}}));
}

// len == 0: there is no type byte. Counted once as bad_length; the bytes of the next
// frame must not be read as a type.
TEST(ItchParserTest, ZeroLengthFrame) {
    std::vector<std::uint8_t> s;
    append(s, '?', 0);
    append(s, 'D', itch::DeleteOrder::length, 9);

    RecordingHandler h;
    const itch::ParseStats st = itch::parse(s.data(), s.size(), h);

    EXPECT_EQ(st.consumed, s.size());
    EXPECT_EQ(st.bad_length, 1u);
    EXPECT_EQ(st.skipped, 0u);
    EXPECT_EQ(st.messages, 1u);
    EXPECT_EQ(h.calls, (std::vector<Call>{{'D', 9}}));
}

// Split the stream at every byte into two calls, carrying the unconsumed tail into the
// second call (as a reader with a fixed buffer would). Result must match a single pass.
TEST(ItchParserTest, SplitAtEveryByteMatchesSinglePass) {
    std::vector<std::uint8_t> s;
    append(s, 'A', itch::AddOrder::length, 1);
    append(s, 'S', itch::SystemEvent::length);
    append(s, 'U', itch::ReplaceOrder::length, 1);
    append(s, 'E', itch::ExecuteOrder::length, 2);
    append(s, 'D', itch::DeleteOrder::length, 2);

    RecordingHandler whole;
    itch::parse(s.data(), s.size(), whole);

    for (std::size_t cut = 0; cut <= s.size(); ++cut) {
        RecordingHandler h;
        const itch::ParseStats first = itch::parse(s.data(), cut, h);
        ASSERT_LE(first.consumed, cut) << "cut=" << cut;

        const std::vector<std::uint8_t> rest(
            s.begin() + static_cast<std::ptrdiff_t>(first.consumed), s.end());
        const itch::ParseStats second = itch::parse(rest.data(), rest.size(), h);

        EXPECT_EQ(first.consumed + second.consumed, s.size()) << "cut=" << cut;
        EXPECT_EQ(h.calls, whole.calls) << "cut=" << cut;
    }
}
