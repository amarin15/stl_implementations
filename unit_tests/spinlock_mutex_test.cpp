#include <gtest/gtest.h>

#include <future>
#include <stack>
#include <thread>
#include <unordered_set>

#include <si_spinlock_mutex.h>

TEST (SpinlockMutexShould, LockAndUnlock)
{
    std::stack<int> s;
    const size_t num_elems = 100;
    for (int i = 0; i < num_elems; i ++)
        s.push(i);

    si::spinlock_mutex sm;
    auto pop = [&sm, &s]() {
        std::unordered_set<int> popped;
        while (true)
        {
            std::unique_lock<std::remove_reference<decltype(sm)>::type> ulock(sm);
            if (s.empty())
                break;
            int val = s.top();
            s.pop();
            ulock.unlock();

            popped.insert(val);
        }

        return popped;
    };

    auto f1 = std::async(std::launch::async, pop);
    auto f2 = std::async(std::launch::async, pop);

    std::unordered_set<int> popped_values;
    popped_values.merge(f1.get());
    popped_values.merge(f2.get());

    EXPECT_EQ(popped_values.size(), num_elems);
}
