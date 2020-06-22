#include <gtest/gtest.h>

#include <future>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <si_threadsafe_unordered_map.h>

// Validate our implementations using the same tests.
TEST(si_threadsafe_unordered_map, test_interface)
{
    si::threadsafe_unordered_map<int, int> m;

    m.insert(1, std::make_shared<int>(1));
    m.insert(1, std::make_shared<int>(2));
    auto it = m.find(1);
    EXPECT_TRUE(it);
    EXPECT_EQ(*m.find(1), 1);
    m.insert_or_update(1, std::make_shared<int>(2));
    EXPECT_EQ(*m.find(1), 2);
    m.erase(1);
    EXPECT_FALSE(m.find(1));
}

TEST(si_threadsafe_unordered_map, has_thread_safe_insert)
{
    using map_type = si::threadsafe_unordered_map<int, std::string>;
    map_type m(10);
    const size_t chunk_size = 25;
    const size_t num_threads = 20;
    const size_t total = chunk_size * num_threads;

    auto insert_string = [](map_type& m, int start)
        {
            for (int i = start; i < start + chunk_size; i ++)
                m.insert(i, std::make_shared<std::string>(std::to_string(i)));
        };

    auto get_string = [](map_type& m, int start)
        {
            std::unordered_set<std::string> values;
            for (int i = start; i < start + chunk_size; i ++)
            {
                auto sptr = m.find(i);
                if (sptr)
                    values.insert(*sptr);
            }

            return values;
        };

    std::vector<std::future<void>> insert_futures;
    for (int i = 0; i < total; i += chunk_size)
        insert_futures.push_back(std::async(std::launch::async, insert_string, std::ref(m), i));

    for (auto& f : insert_futures)
        f.get();

    std::vector<std::future<std::unordered_set<std::string>>> get_futures;
    for (int i = 0; i < total; i += chunk_size)
        get_futures.push_back(std::async(std::launch::async, get_string, std::ref(m), i));

    // Check no values are skipped when popping from 2 different threads.
    // (multiple threads can never call front() on the same value and then pop different values)
    std::unordered_set<std::string> popped_values;
    for (auto& f : get_futures)
        popped_values.merge(f.get());

    EXPECT_EQ(popped_values.size(), total);
}
