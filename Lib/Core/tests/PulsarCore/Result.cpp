#include <PulsarCore/Result.hpp>
#include <gtest/gtest.h>

using namespace Pulsar;

// TODO: Add more tests, but since we are using std::expected, we can't really

TEST(Result, Basic) {
    Result<int, int> res = Result<int, int>(42);
    EXPECT_EQ(res.has_value(), true);
    EXPECT_EQ(res.value(), 42);
}

TEST(Result, BasicError) {
    Result<int, int> res = Result<int, int>(Err<int>(42));
    EXPECT_EQ(res.has_value(), false);
    EXPECT_EQ(res.error(), 42);
}
