/* Placeholder test suite */
#include <gtest/gtest.h>

class BasicTest : public ::testing::Test {};

TEST_F(BasicTest, Placeholder) {
    EXPECT_EQ(1, 1);
}
