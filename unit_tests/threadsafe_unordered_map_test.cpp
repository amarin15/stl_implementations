#include <gtest/gtest.h>

#include <string>

#include <si_threadsafe_unordered_map.h>

// Validate our implementations using the same tests.
TEST(si_threadsafe_unordered_map, test_interface)
{
    si::threadsafe_unordered_map<int, int> m;

    m[1] = 1;
    m[1] = 2;

    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m[1], 2);

    if (m[4] == 4)
        ;
    EXPECT_EQ(m.size(), 2);

    m.erase(4);
    EXPECT_EQ(m.size(), 1);
}
