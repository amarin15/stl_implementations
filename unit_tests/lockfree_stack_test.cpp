#include <gtest/gtest.h>

#include <si_lockfree_stack.h>

TEST(LockfreeStackShould, SupportPushAndPop)
{
    si::lockfree_stack<int> s;
    s.push(42);
    auto ptr = s.pop();
    ASSERT_TRUE(ptr);
    EXPECT_EQ(42, *ptr);
}
