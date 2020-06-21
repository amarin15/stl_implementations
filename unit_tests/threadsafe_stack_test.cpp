#include <gtest/gtest.h>

#include <future>
#include <string>
#include <thread>
#include <unordered_set>

#include <si_threadsafe_stack.h>

TEST(ThreadsafeStackShould, SupportEmptyAndPush)
{
    si::threadsafe_stack<int> s;
    EXPECT_TRUE(s.empty());

    s.push(42);
    EXPECT_FALSE(s.empty());
}

TEST(ThreadsafeStackShould, PopByReference)
{
    // GIVEN
    si::threadsafe_stack<int> s;
    s.push(42);

    // WHEN
    int val;
    s.pop(val);

    // THEN
    EXPECT_EQ(val, 42);
}

TEST(ThreadsafeStackShould, PopBySharedPtr)
{
    std::string val("test");
    si::threadsafe_stack<std::string> s;
    s.push(val);

    auto str_sptr = s.pop();

    ASSERT_TRUE(str_sptr);
    EXPECT_EQ(*str_sptr, val);
}

TEST(ThreadsafeStackShould, ThrowWhenPopOnEmpty)
{
    si::threadsafe_stack<int> s;
    EXPECT_THROW(s.pop(), std::runtime_error);
    EXPECT_THROW({
        int val;
        s.pop(val);
    }, std::runtime_error);
}

TEST(ThreadsafeStackShould, BeThreadSafe)
{
    si::threadsafe_stack<int> s;
    for (int i = 0; i < 10; ++ i)
        s.push(i);

    // Check no values are skipped when popping from 2 different threads.
    // (multiple threads can never call top() on the same value and then pop different values)
    std::unordered_set<int> popped_values;

    auto pop_sptr = [](si::threadsafe_stack<int>& ts)
        {
            std::unordered_set<int> vals;
            while (true)
            {
                try
                {
                    auto sptr = ts.pop();
                    vals.insert(*sptr);
                }
                catch(const std::runtime_error& err)
                {
                    break;
                }
            }

            return vals;
        };

    auto pop_ref = [](si::threadsafe_stack<int>& ts)
        {
            std::unordered_set<int> vals;
            while (true)
            {
                try
                {
                    int val;
                    ts.pop(val);
                    vals.insert(val);
                }
                catch(const std::runtime_error& err)
                {
                    break;
                }
            }

            return vals;
        };

    auto f_sptr = std::async(std::launch::async, pop_sptr, std::ref(s));
    auto f_ref  = std::async(std::launch::async, pop_ref , std::ref(s));

    popped_values.merge(f_sptr.get());
    popped_values.merge(f_ref.get());

    EXPECT_EQ(popped_values.size(), 10);
}
