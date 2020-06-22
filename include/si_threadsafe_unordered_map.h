#ifndef SI_THREADSAFE_UNORDERED_MAP_H
#define SI_THREADSAFE_UNORDERED_MAP_H

#include <list>
#include <mutex>
#include <utility>
#include <vector>

namespace si {

template<
    typename Key
  , typename T
  , typename Hash = std::hash<Key>
> class threadsafe_unordered_map
{
public:
    threadsafe_unordered_map<Key, T>(size_t num_buckets = 4, double max_load_factor = 0.8)
        : d_size(0)
        , d_max_load_factor(max_load_factor)
        , d_buckets(num_buckets)
        , d_locks(num_buckets)
        , d_size_lock()
    {}

    typename std::list<std::pair<Key, T>>::iterator insert(const Key& k, const T& val)
    {
        size_t bucket = get_bucket(k);

        std::unique_lock<std::mutex> ulock(d_locks[bucket]);

        // don't insert if already present.
        // needs to be locked as well, so a different thread doesn't insert the same element
        // between this check and the following insert.
        auto &b = d_buckets[bucket];
        for (auto it = b.begin(); it != b.end(); ++it)
            if (it->first == k)
                return it;

        if ((d_size + 1) / d_buckets.size() > d_max_load_factor)
            rehash();

        d_buckets[bucket].push_back(std::make_pair(k, val));
        ulock.unlock();

        // make sure there are no race conditions between reading the value of the size
        // and updating it with the new size
        std::lock_guard<std::mutex> guard(d_size_lock);
        d_size ++;
        return std::prev(d_buckets[bucket].end());
    }

    T& operator[](const Key& k)
    {
        auto it = insert(k, T());
        return it->second;
    }

    bool erase(const Key& k)
    {
        size_t bucket = get_bucket(k);

        auto &b = d_buckets[bucket];
        for (auto it = b.begin(); it != b.end(); ++it)
        {
            if (it->first == k)
            {
                std::unique_lock<std::mutex> ulock(d_locks[bucket]);
                d_buckets[bucket].erase(it);
                ulock.unlock();

                std::lock_guard<std::mutex> guard(d_size_lock);
                d_size--;
                return true;
            }
        }

        return false;
    }

    void rehash()
    {
        // not implemented
    }

    size_t size() const noexcept
    {
        return d_size;
    }

private:
    size_t get_bucket(const Key& k) const
    {
        return Hash()(k) % d_buckets.size();;
    }

    size_t d_size;
    double d_max_load_factor;
    std::vector<std::list<std::pair<Key, T>>> d_buckets;
    std::vector<std::mutex> d_locks;
    std::mutex d_size_lock;
};

} // namespace si

#endif
