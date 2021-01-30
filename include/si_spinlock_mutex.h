#ifndef SI_SPINLOCK_MUTEX_H
#define SI_SPINLOCK_MUTEX_H

#include <atomic>
#include <immintrin.h> // for _mm_pause

namespace si {

// Notes on why this is a terrible spinlock and why the amd version is 10x faster
// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
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

// Optimizations based on the AMD slides in the link above
struct spinlock_amd
{
    void lock()
    {
        for (;;)
        {
            // Loads the memory before attempting to make a change to it.
            // In the MESI protocol this means that the cache line can be in
            // the “shared” state on all cores which requires no communication
            // between the CPU cores until the data actually changes.
            bool was_locked = locked.load(std::memory_order_relaxed);
            if (!was_locked && locked.compare_exchange_weak(was_locked, true, std::memory_order_acquire))
                break;
            // CPU uses this as a hint that the other hyper-thread running
            // on the same core should run instead of this thread.
            _mm_pause();
        }
    }
    void unlock()
    {
        locked.store(false, std::memory_order_release);
    }
 
private:
    std::atomic<bool> locked{false};
};

} // namespace si

#endif
