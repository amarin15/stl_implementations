#ifndef SI_THREADSAFE_QUEUE_H
#define SI_THREADSAFE_QUEUE_H

#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace si {


template <class T>
class threadsafe_queue
{
public:
    threadsafe_queue()
    {}

    threadsafe_queue(const threadsafe_queue<T>& other)
    {
        std::lock_guard<std::mutex> guard(other.d_mutex);
        d_queue = other.d_queue;
    }

    threadsafe_queue& operator= (const threadsafe_queue<T>& other) = delete;

    void push(const T& val)
    {
        std::unique_lock<std::mutex> ulock(d_mutex);
        d_queue.push(val);
        ulock.unlock();

        d_cond.notify_one();
    }

    void wait_and_pop(T& result)
    {
        std::unique_lock<std::mutex> ulock(d_mutex);
        d_cond.wait(ulock, [this](){ return !empty(); });

        result = d_queue.front();
        d_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> ulock(d_mutex);
        d_cond.wait(ulock, [this](){ return !empty(); });

        auto ret = std::make_shared<T>(d_queue.front());
        d_queue.pop();
        return ret;
    }

    bool try_pop(T& result)
    {
        std::lock_guard<std::mutex> guard(d_mutex);
        if (empty())
            return false;

        result = d_queue.front();
        d_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> guard(d_mutex);
        if (empty())
            return nullptr;

        auto ret = std::make_shared<T>(d_queue.front());
        d_queue.pop();
        return ret;
    }

    bool empty() const noexcept
    {
        return d_queue.empty();
    }

private:
    std::queue<T>           d_queue;
    mutable std::mutex      d_mutex;
    std::condition_variable d_cond;
};


} // namespace si

#endif

