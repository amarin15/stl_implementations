#include <gtest/gtest.h>

#include <string>

#include <si_threadsafe_unordered_map.h>

// Validate our implementations using the same tests.
TEST(si_threadsafe_unordered_map, test_interface)
{
    si::threadsafe_unordered_map<int, int> m;

    m.insert(1, 1);
    m.insert(1, 2);
    EXPECT_EQ(m.find(1), 1);
    m.insert_or_update(1, 2);
    EXPECT_EQ(m.find(1), 2);
    m.erase(1);
    EXPECT_EQ(m.find(1), 0);
    EXPECT_EQ(m.find(1, -100), -100);
}
