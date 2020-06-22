#include <gtest/gtest.h>

#include <memory>

#include <si_threadsafe_unordered_map.h>

// Validate our implementations using the same tests.
TEST(si_threadsafe_unordered_map, test_interface)
{
    si::threadsafe_unordered_map<int, int> m;

    m.insert(1, std::make_shared<int>(1));
    m.insert(1, std::make_shared<int>(2));
    EXPECT_EQ(*m.find(1), 1);
    m.insert_or_update(1, std::make_shared<int>(2));
    EXPECT_EQ(*m.find(1), 2);
    m.erase(1);
    EXPECT_FALSE(m.find(1));
}

TEST(si_threadsafe_unordered_map, is_thread_safe)
{
    // TODO
}