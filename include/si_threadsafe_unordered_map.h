#ifndef SI_THREADSAFE_UNORDERED_MAP_H
#define SI_THREADSAFE_UNORDERED_MAP_H

#include <list>
#include <memory>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace si {

// Hide the bucket type in an anonymous namespace so it's only visible
// in this translation unit. If we made it a private class inside the map
// then we would have one bucket_type class for each specialization of Hash.
namespace {

template <typename Key, typename Value>
class bucket
{
public:
    std::shared_ptr<Value> find(const Key& k) const
    {
        std::shared_lock<std::shared_mutex> slock(d_mutex);
        for (const auto& p : d_nodes)
            if (p.first == k)
                return p.second;

        return nullptr;
    }

    void insert(const Key& k, const std::shared_ptr<Value>& val)
    {
        std::lock_guard<std::shared_mutex> guard(d_mutex);
        for (const auto& p : d_nodes)
            if (p.first == k)
                return;

        d_nodes.emplace_back(k, val);
    }

    void insert_or_update(const Key& k, const std::shared_ptr<Value>& val)
    {
        std::lock_guard<std::shared_mutex> guard(d_mutex);
        for (auto& p : d_nodes)
        {
            if (p.first == k)
            {
                p.second = val;
                return;
            }
        }

        d_nodes.emplace_back(k, val);
    }

    void erase(const Key& k)
    {
        std::lock_guard<std::shared_mutex> guard(d_mutex);
        for (auto it = d_nodes.begin(); it != d_nodes.end(); ++ it)
        {
            if (it->first == k)
            {
                d_nodes.erase(it);
                return;
            }
        }
    }

private:
    // Hold shared_ptrs so we avoid copying large value objects.
    // Also allows the bucket to hold objects that are not copyable.
    std::list<std::pair<Key, std::shared_ptr<Value>>> d_nodes;
    mutable std::shared_mutex                         d_mutex;
};

} // close anonymous namespace

template<
    typename Key
  , typename Value
  , typename Hash = std::hash<Key>
> class threadsafe_unordered_map
{
public:
    threadsafe_unordered_map<Key, Value>(size_t num_buckets = 5)
        : d_buckets(num_buckets)
        , d_hasher(Hash())
    {}

    // If the value is not found, return default_value
    std::shared_ptr<Value> find(const Key& k) const
    {
        return get_bucket(k).find(k);
    }

    // Don't return references or iterators to avoid race conditions
    void insert(const Key& k, const std::shared_ptr<Value>& val)
    {
        get_bucket(k).insert(k, val);
    }

    void insert_or_update(const Key& k, const std::shared_ptr<Value>& val)
    {
        get_bucket(k).insert_or_update(k, val);
    }

    void erase(const Key& k)
    {
        get_bucket(k).erase(k);
    }

private:
    using bucket_type = bucket<Key, Value>;

    bucket_type& get_bucket(const Key& k)
    {
        return d_buckets[ d_hasher(k) % d_buckets.size() ];
    }

    std::vector<bucket_type> d_buckets;
    Hash                     d_hasher;
};

} // namespace si

#endif
