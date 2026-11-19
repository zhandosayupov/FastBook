#include <fastbook/utils/byte_order.h>
#include <gtest/gtest.h>

TEST(ByteOrderTest, ReadUint32BE) {
    std::uint8_t data[4] = {0x12, 0x34, 0x56, 0x78};
    std::uint32_t value = fastbook::utils::read_uint32_be(data);
    EXPECT_EQ(value, 0x12345678);
}

TEST(ByteOrderTest, ReadUint64BE) {
    std::uint8_t data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    std::uint64_t value = fastbook::utils::read_uint64_be(data);
    EXPECT_EQ(value, 0x123456789ABCDEF0);
}

TEST(ByteOrderTest, ReadUint16BE) {
    std::uint8_t data[2] = {0x12, 0x34};
    std::uint16_t value = fastbook::utils::read_uint16_be(data);
    EXPECT_EQ(value, 0x1234);
}

TEST(ByteOrderTest, ReadUint48BE) {
    std::uint8_t data[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    std::uint64_t value = fastbook::utils::read_uint48_be(data);
    EXPECT_EQ(value, 0x123456789ABC);
}

TEST(ByteOrderTest, ReadUint32BE_Zero) {
    std::uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};
    std::uint32_t value = fastbook::utils::read_uint32_be(data);
    EXPECT_EQ(value, 0x00000000);
}

TEST(ByteOrderTest, ReadUint64BE_Zero) {
    std::uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    std::uint64_t value = fastbook::utils::read_uint64_be(data);
    EXPECT_EQ(value, 0x0000000000000000);
}

TEST(ByteOrderTest, ReadUint16BE_Zero) {
    std::uint8_t data[2] = {0x00, 0x00};
    std::uint16_t value = fastbook::utils::read_uint16_be(data);
    EXPECT_EQ(value, 0x0000);
}

TEST(ByteOrderTest, ReadUint48BE_Zero) {
    std::uint8_t data[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    std::uint64_t value = fastbook::utils::read_uint48_be(data);
    EXPECT_EQ(value, 0x000000000000);
}
