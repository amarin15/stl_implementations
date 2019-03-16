#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include <si_spmc_queue.h>


template<typename T, typename ... Args>
void produce(si::spmc_fifo_queue<T>& q, unsigned to_produce, Args&& ... args)
{
    for (unsigned i = 0; i < to_produce; ++i)
        q.push(i, std::forward<Args>(args)...);
}

template<typename T>
void consume(si::spmc_fifo_queue<T>& q, unsigned to_consume)
{
    std::set<T> s;
    for (unsigned i = 0; i < to_consume; ++i)
        s.insert(q.pop());
    EXPECT_EQ(to_consume, s.size());
}

TEST(spmc_fifo_queue, produce_and_consume)
{
    using T = std::string;
    si::spmc_fifo_queue<T> q;
    const unsigned num_consumers = 12;
    const unsigned per_consumer = 10;
    const unsigned per_producer = num_consumers * per_consumer;

    std::thread producer(produce<T, char>, std::ref(q), per_producer, 'a');

    std::vector<std::thread> consumers;
    for (unsigned i = 0; i < num_consumers; ++i)
        consumers.push_back(std::thread(consume<T>, std::ref(q), per_consumer));

    producer.join();
    for (unsigned i = 0; i < num_consumers; ++i)
        consumers[i].join();
}

