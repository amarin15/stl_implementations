#ifndef SI_SPMC_QUEUE_H
#define SI_SPMC_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace si {

// Single-producer multiple-consumer queue.
template <typename T>
class spmc_fifo_queue
{
public:
    template <typename ... Args>
    void push(Args&&... args)
    {
        std::unique_lock<std::mutex> lock_(d_mutex);
        d_queue.emplace(std::forward<Args>(args)...);
        lock_.unlock();

        d_cond.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock_(d_mutex);
        d_cond.wait(lock_, [this](){ return !d_queue.empty(); });

        T elem = std::move(d_queue.front());
        d_queue.pop();

        return elem;
    }

private:
    // DATA
    std::queue<T>           d_queue;
    std::mutex              d_mutex;
    std::condition_variable d_cond;
};

} // namespace si

#endif

