#include <fastbook/itch/messages.h>
#include <gtest/gtest.h>

TEST(ItchMessagesTest, AddOrderLength) { EXPECT_EQ(fastbook::itch::AddOrder::length, 36u); }
