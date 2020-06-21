#ifndef SI_SPINLOCK_MUTEX_H
#define SI_SPINLOCK_MUTEX_H

#include <atomic>

namespace si {

class spinlock_mutex
{
public:
    void lock() noexcept
    {
        while (d_flag.test_and_set(std::memory_order_acquire))
            ; // busy waiting
    }

    void unlock() noexcept
    {
        d_flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag d_flag = ATOMIC_FLAG_INIT;
};

} // namespace si

#endif
