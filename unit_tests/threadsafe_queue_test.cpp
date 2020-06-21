#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <unordered_set>

#include <si_threadsafe_queue.h>

TEST(ThreadsafeQueueShould, SupportEmptyAndPush)
{
    si::threadsafe_queue<int> q;
    EXPECT_TRUE(q.empty());

    q.push(42);
    EXPECT_FALSE(q.empty());
}

TEST(ThreadsafeQueueShould, WaitAndPopEmptyByReference)
{
    // GIVEN
    si::threadsafe_queue<int> q;

    // WHEN
    auto fn = [](si::threadsafe_queue<int>& q, int& val) { q.wait_and_pop(val); };
    int val = 0;
    std::thread t(fn, std::ref(q), std::ref(val));

    // give t some time to run (make sure it waits)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // now push the value
    q.push(42);
    // give t some time to act on the notification
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    // THEN
    EXPECT_EQ(val, 42);

    // wait for t to finish
    t.join();
}

TEST(ThreadsafeQueueShould, WaitAndPopNotEmptyByReference)
{
    // GIVEN
    si::threadsafe_queue<int> q;
    q.push(42);

    // WHEN
    int val;
    q.wait_and_pop(val);

    // THEN
    EXPECT_EQ(val, 42);
}

TEST(ThreadsafeQueueShould, WaitAndPopEmptyBySharedPtr)
{
    // GIVEN
    si::threadsafe_queue<std::string> q;
    std::string val("test");

    // WHEN
    auto fn = [](si::threadsafe_queue<std::string>& q) { return q.wait_and_pop(); };
    // start a new thread immediately to pop async
    auto f = std::async(std::launch::async, fn, std::ref(q));

    // give the new thread some time to run (make sure it waits)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // now push the value and notify
    q.push(val);
    // wait until the future has a valid result
    auto sptr = f.get();

    // THEN
    ASSERT_TRUE(sptr);
    EXPECT_EQ(*sptr, val);
}

TEST(ThreadsafeQueueShould, WaitAndPopNotEmptyBySharedPtr)
{
    // GIVEN
    si::threadsafe_queue<std::string> q;
    std::string val("test");
    q.push(val);

    // WHEN
    auto sptr = q.wait_and_pop();

    // THEN
    ASSERT_TRUE(sptr);
    EXPECT_EQ(*sptr, val);
}

TEST(ThreadsafeQueueShould, TryPopEmptyByReference)
{
    // GIVEN
    si::threadsafe_queue<int> q;

    // WHEN
    int val;
    bool success = q.try_pop(val);

    // THEN
    EXPECT_FALSE(success);
}

TEST(ThreadsafeQueueShould, TryPopNotEmptyByReference)
{
    // GIVEN
    si::threadsafe_queue<int> q;
    q.push(42);

    // WHEN
    int val;
    bool success = q.try_pop(val);

    // THEN
    EXPECT_TRUE(success);
    EXPECT_EQ(val, 42);
}

TEST(ThreadsafeQueueShould, TryPopEmptyBySharedPtr)
{
    // GIVEN
    si::threadsafe_queue<std::string> q;

    // WHEN
    auto sptr = q.try_pop();

    // THEN
    EXPECT_FALSE(sptr);
}

TEST(ThreadsafeQueueShould, TryPopNotEmptyBySharedPtr)
{
    // GIVEN
    si::threadsafe_queue<std::string> q;
    std::string val("test");
    q.push(val);

    // WHEN
    auto sptr = q.try_pop();

    // THEN
    ASSERT_TRUE(sptr);
    EXPECT_EQ(*sptr, val);
}

TEST(ThreadsafeQueueShould, BeThreadSafe)
{
    si::threadsafe_queue<int> q;
    for (int i = 0; i < 10; ++ i)
        q.push(i);

    // Check no values are skipped when popping from 2 different threads.
    // (multiple threads can never call front() on the same value and then pop different values)
    std::unordered_set<int> popped_values;

    auto pop_sptr = [](si::threadsafe_queue<int>& tq)
        {
            std::unordered_set<int> vals;
            while (true)
            {
                auto sptr = tq.try_pop();
                if (!sptr)
                    break;
                vals.insert(*sptr);
            }

            return vals;
        };

    auto pop_ref = [](si::threadsafe_queue<int>& tq)
        {
            std::unordered_set<int> vals;
            int val;
            bool not_empty = true;
            while (not_empty)
            {
                not_empty = tq.try_pop(val);
                vals.insert(val);
            }

            return vals;
        };

    auto f_sptr = std::async(std::launch::async, pop_sptr, std::ref(q));
    auto f_ref  = std::async(std::launch::async, pop_ref , std::ref(q));

    popped_values.merge(f_sptr.get());
    popped_values.merge(f_ref.get());

    EXPECT_EQ(popped_values.size(), 10);
}
