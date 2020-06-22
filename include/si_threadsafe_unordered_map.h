#ifndef SI_THREADSAFE_UNORDERED_MAP_H
#define SI_THREADSAFE_UNORDERED_MAP_H

#include <list>
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
    Value find(const Key& k, const Value& default_val) const noexcept
    {
        for (const auto& p : d_nodes)
            if (p.first == k)
                return p.second;
        return default_val;
    }

    void insert(const Key& k, const Value& val)
    {
        for (const auto& p : d_nodes)
            if (p.first == k)
                return;
        d_nodes.push_back(std::make_pair(k, val));
    }

    void insert_or_update(const Key& k, const Value& val)
    {
        for (auto& p : d_nodes)
            if (p.first == k)
            {
                p.second = val;
                return;
            }
        d_nodes.push_back(std::make_pair(k, val));
    }

    void erase(const Key& k)
    {
        for (auto it = d_nodes.begin(); it != d_nodes.end(); ++ it)
            if (it->first == k)
            {
                d_nodes.erase(it);
                return;
            }
    }

private:
    std::list<std::pair<Key, Value>> d_nodes;
    mutable std::shared_mutex        d_mutex;
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
    Value find(const Key& k, const Value& default_value = Value())
    {
        return get_bucket(k).find(k, default_value);
    }

    void insert(const Key& k, const Value& val)
    {
        get_bucket(k).insert(k, val);
    }

    void insert_or_update(const Key& k, const Value& val)
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
        return d_buckets[d_hasher(k) % d_buckets.size()];
    }

    std::vector<bucket_type>  d_buckets;
    Hash                      d_hasher;
};

} // namespace si

#endif
