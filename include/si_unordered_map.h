#ifndef SI_UNORDERED_MAP_H
#define SI_UNORDERED_MAP_H

#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include <vector>


namespace si {
template<typename Key, typename T, typename Hash, typename KeyEqual>
class unordered_map;
} // namespace si

namespace std {
template <typename Key, typename T, typename Hash, typename KeyEqual>
void swap(si::unordered_map<Key, T, Hash, KeyEqual>& lhs
        , si::unordered_map<Key, T, Hash, KeyEqual>& rhs)
{
    lhs.swap(rhs);
}
} // namespace std


namespace si {

template<
    typename Key
  , typename T
  , typename Hash = std::hash<Key>
  , typename KeyEqual = std::equal_to<Key>
> class unordered_map
{
    // std::is_invocable is only present in C++17, everything else compiles
    // with C++11. You can find a C+11 implementation of is_invocable here:
    // https://github.com/gcc-mirror/gcc/blob/d3a3029ca7489cb168d493de3d695809e84ffb0f/libstdc%2B%2B-v3/include/std/type_traits#L2661
    static_assert(std::is_invocable<Hash, Key>::value
                 , "std::hash must be invocable with a Key type");

    // Forward declarations
    struct _Iterator;
    struct _ConstIterator;


public:
    // ~~ Types ~~

    typedef Key                     key_type;
    typedef T                       mapped_type;
    typedef std::pair<const Key, T> value_type;
    typedef Hash                    hasher;
    typedef KeyEqual                key_equal;
    typedef _Iterator               iterator;
    typedef _ConstIterator          const_iterator;


private:
    // ~~ Internal classes ~~

    // Curiously recurring template pattern.
    // `d_before_begin` is just a sentinel and doesn't need a
    // value_type member, so we can use a base class for it.
    template<typename _NodeType>
    struct _NodeBase
    {
        _NodeType*  next;

        explicit _NodeBase(_NodeType* np) noexcept
            : next(np)
        {}

        _NodeBase<_NodeType>& operator=(const _NodeBase<_NodeType>& other) = default;

        // we might destroy a _NodeType* through a _NodeBase<_NodeType>*
        virtual ~_NodeBase<_NodeType>()
        {}
    };

    struct _Node : public _NodeBase<_Node>
    {
        unordered_map::value_type value;

        _Node(_Node* node, unordered_map::value_type&& val)
            : _NodeBase<_Node>(node), value(std::move(val))
        {}
    };

    class _IteratorBase
    {
    public:
        // LegacyForwardIterator
        typedef std::forward_iterator_tag   iterator_category;
        typedef unordered_map::value_type   value_type;
        typedef std::ptrdiff_t              difference_type;

        _Node* cur;

    protected:
        _IteratorBase(_Node* node) noexcept
            : cur(node)
        {}

        // caller's job to check for nullptr
        void increment() noexcept
        {
            cur = cur->next;
        }

    public:
        operator bool() const noexcept
        {
            return cur != nullptr;
        }

        bool operator==(const _IteratorBase& other) const noexcept
        {
            // compare pointers, not values
            return cur == other.cur;
        }

        bool operator!=(const _IteratorBase& other) const noexcept
        {
            return cur != other.cur;
        }
    };

    struct _Iterator : public _IteratorBase
    {
        typedef value_type* pointer;
        typedef value_type& reference;

        explicit _Iterator(_Node* node) noexcept
            : _IteratorBase(node)
        {}

        // caller's job to check the below interface
        // is not called on nullptr (unordered_map.end())
        reference operator*() const noexcept
        {
            return this->cur->value;
        }

        pointer operator->() const noexcept
        {
            return &this->cur->value;
        }

        _Iterator& operator++() noexcept
        {
            this->increment();
            return *this;
        }
    };

    struct _ConstIterator : public _IteratorBase
    {
        typedef const value_type* pointer;
        typedef const value_type& reference;

        explicit _ConstIterator(_Node* node) noexcept
            : _IteratorBase(node)
        {}

        // caller's job to check the below interface
        // is not called on nullptr (unordered_map.end())
        reference operator*() const noexcept
        {
            return this->cur->value;
        }

        pointer operator->() const noexcept
        {
            return &this->cur->value;
        }

        _ConstIterator& operator++() noexcept
        {
            this->increment();
            return *this;
        }
    };


private:
    // ~~ Data ~~

    size_t                          d_bucket_count;
    size_t                          d_size;
    float                           d_max_load_factor;
    _NodeBase<_Node>                d_before_begin;
    // Each bucket has a sentinel, which is either &d_before_begin
    // for the first bucket (most recently created) or the last node
    // from a different bucket (first one to be inserted there) that
    // points to the current bucket. So all sentinels are in fact of
    // type _Node*, except for &d_before_begin, but we only need their
    // .next, so we can use them as _NodeBase<_Node>*.
    //
    // Empty buckets will have nullptr.
    std::vector<_NodeBase<_Node>*>  d_buckets;

    hasher                          d_hasher;
    key_equal                       d_key_equal;

public:
    // ~~ Constructors ~~


    // (1) default constructor
    // No allocation for the table's elements is made.
    explicit unordered_map(size_t bucket_count = 1)
      : d_bucket_count(bucket_count)
      , d_size(0)
      , d_max_load_factor(1)
      , d_before_begin(nullptr)
      , d_buckets(d_bucket_count, nullptr)
    {}

    // (2) range constructor
    template <typename InputIt>
    unordered_map(InputIt first
          , InputIt last
          , size_t bucket_count = 0
          , const Hash& hash = Hash()
          , const key_equal& key_eq = key_equal()
    ) : d_bucket_count(bucket_count == 0 ? std::distance(first, last) : bucket_count)
      , d_size(0)
      , d_max_load_factor(1)
      , d_before_begin(nullptr)
      , d_buckets(d_bucket_count, nullptr)
      , d_hasher(hash)
      , d_key_equal(key_eq)
    {
        insert(first, last);
    }

    // (3) copy constructor
    unordered_map(const unordered_map& other)
        : d_bucket_count(other.bucket_count())
        , d_size(0)
        , d_max_load_factor(other.max_load_factor())
        , d_before_begin(nullptr)
        , d_buckets(d_bucket_count, nullptr)
    {
        // use insert to allocate new memory
        // instead of copying pointers.
        insert(other.cbegin(), other.cend());
    }

    // (4) move constructor
    unordered_map(unordered_map&& other)
        : d_bucket_count(other.d_bucket_count)
        , d_size(other.d_size)
        , d_max_load_factor(other.d_max_load_factor)
        , d_before_begin(std::move(other.d_before_begin))
        , d_buckets(std::move(other.d_buckets))
    {
        // avoid deleting the nodes which we moved from other
        other.d_size = 0;
    }

    // (5) initializer list
    unordered_map(const std::initializer_list<value_type>& init
          , size_t bucket_count = 0
    ) : d_bucket_count(bucket_count == 0 ? init.size() : bucket_count)
      , d_size(0)
      , d_max_load_factor(1)
      , d_before_begin(nullptr)
      , d_buckets(d_bucket_count, nullptr)
    {
        insert(init);
    }

    ~unordered_map()
    {
        _clear();
    }


    unordered_map<Key, T>& operator=(const unordered_map<Key, T>& other)
    {
        // clear() doesn't change d_bucket_count, but uses it to reset d_buckets
        d_bucket_count = other.bucket_count();
        clear();
        insert(other.cbegin(), other.cend());

        return *this;
    }

    unordered_map<Key, T>& operator=(unordered_map<Key, T>&& other)
    {
        _clear();
        d_bucket_count    = other.d_bucket_count;
        d_size            = other.d_size;
        d_max_load_factor = other.d_max_load_factor;
        d_before_begin    = std::move(other.d_before_begin);
        d_buckets         = std::move(other.d_buckets);
        d_hasher          = std::move(other.d_hasher);
        d_key_equal       = std::move(other.d_key_equal);

        // prevent deleting the nodes we moved from other
        other.d_size = 0;

        return *this;
    }


    // ~~ Iterators ~~

    iterator begin() noexcept
    {
        return iterator(d_before_begin.next);
    }

    iterator end() noexcept
    {
        return iterator(nullptr);
    }

    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    const_iterator end() const noexcept
    {
        return cend();
    }

    const_iterator cbegin() const noexcept
    {
        return const_iterator(d_before_begin.next);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator(nullptr);
    }


    // ~~ Capacity ~~

    size_t empty() const noexcept
    {
        return d_size == 0;
    }

    size_t size() const noexcept
    {
        return d_size;
    }

    size_t max_size() const noexcept
    {
        // could potentially find a better implementation.
        // ptrdiff_t is signed
        return std::numeric_limits<ptrdiff_t>::max();
    }


    // ~~ Modifiers ~~

    void clear() noexcept
    {
        _clear(); // don't leak memory!
        d_size = 0;
        d_before_begin.next = nullptr;
        // leaves d_bucket_count unchanged
        d_buckets = std::vector<_NodeBase<_Node>*>(d_bucket_count, nullptr);
    }


    // (1) copy value
    std::pair<iterator, bool> insert(const value_type& val)
    {
        return _insert(val);
    }

    // (1) move value
    std::pair<iterator, bool> insert(value_type&& val)
    {
        return _insert(std::move(val));
    }

    // (2) emplace value
    template < typename P
             , typename NotUsed = typename std::enable_if<
                // Only allow forwarding references that value_type can be
                // constructed from.
                std::is_constructible<value_type, P&&>::value
                // Overload resolution makes sure that (1) takes priority over
                // (2) when P is value_type, because it matches it more closely,
                // which means we don't make an extra copy calling emplace.
                // That makes the below condition redundant:
                // && !std::is_same<P, value_type>::value
              >::type >
    std::pair<iterator, bool> insert(P&& p)
    {
        return emplace(std::forward<P>(p));
    }

    // (5) range
    template<typename InputIt>
    void insert(InputIt first, InputIt last)
    {
        while (first != last)
        {
            _insert(*first);
            ++first;
        }
    }

    // (6) initializer_list elements are always const, so this calls
    // _insert(const value_type&).
    void insert(const std::initializer_list<value_type>& init)
    {
        for (auto it = init.begin(); it != init.end(); it++)
            _insert(*it);
    }

    template <typename ... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        // Construct value_type from both lvalues or rvalues
        // and call _insert with an rvalue.
        return _insert(value_type(std::forward<Args>(args)...));
    }


    // The iterator pos must be valid and dereferenceable.
    iterator erase(const const_iterator& pos)
    {
        return _erase(pos.cur->value.first);
    }

    size_t erase(const key_type& key)
    {
        return bool(_erase(key));
    }

    // Erase range, potentially across different buckets.
    iterator erase(const_iterator first, const_iterator last)
    {
        if (first)
        {
            // Only need to get the sentinel once in order to
            // find the element before first.
            _NodeBase<_Node>* before_first = _sentinel(*first);

            // Find first element before it.
            // If first is valid, so should be the sentinel.
            const_iterator cur_cit = _ConstIterator(before_first->next);
            while (cur_cit && cur_cit != first)
            {
                ++ cur_cit;
                before_first = before_first->next;
            }

            if (cur_cit == first)
            {
                // delete all elements between first and last
                while (cur_cit != last)
                {
                    _Node* to_delete = cur_cit.cur;
                    size_t bucket_num_cur = bucket(cur_cit->first);

                    ++ cur_cit;
                    delete to_delete;
                    -- d_size;

                    // We always delete the first element in a bucket. If its next is
                    // in a different bucket, it means it's also the last, in which case
                    // this bucket will remain empty, so we need to delete the sentinel.
                    if (cur_cit)
                    {
                        size_t bucket_num_next = bucket(cur_cit->first);
                        if (bucket_num_next != bucket_num_cur)
                            d_buckets[bucket_num_cur] = nullptr;
                    }
                    else
                        break;
                }

                // point next of node before first to last
                if (last && cur_cit == last)
                    before_first->next = last.cur;
                else
                    before_first->next = nullptr;
            }
        }

        return iterator(last.cur);
    }


    void swap(unordered_map& other) noexcept
    {
        std::swap(d_bucket_count, other.d_bucket_count);
        std::swap(d_size, other.d_size);
        std::swap(d_max_load_factor, other.d_max_load_factor);
        std::swap(d_before_begin, other.d_before_begin);
        std::swap(d_buckets, other.d_buckets);
    }


    // ~~ Lookup ~~

    const T& at(const Key& key) const
    {
        _Node* res = _find_node_ptr(key);
        if (res == nullptr)
            throw std::out_of_range("Key not found.");

        return res->value.second;
    }

    // Use const implementation and remove constness.
    T& at(const Key& key)
    {
        return const_cast<T&>(static_cast<const unordered_map<T, Key>*>(this)->at(key));
    }

    T& operator[](Key&& key)
    {
        auto p = _insert(value_type(std::move(key), T()));
        return p.first->second;
    }

    T& operator[](const Key& key)
    {
        return this->operator[](Key(key));
    }

    size_t count(const Key& key) const
    {
        return bool(_find_node_ptr(key));
    }

    iterator find(const Key& key)
    {
        return iterator(_find_node_ptr(key));
    }

    const_iterator find(const Key& key) const
    {
        return const_iterator(_find_node_ptr(key));
    }

    std::pair<iterator, iterator> equal_range(const Key& key)
    {
        _Node* cur = _find_node_ptr(key);
        return std::make_pair(iterator(cur)
                            , iterator(cur == nullptr ? nullptr : cur->next));
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
    {
        _Node* cur = _find_node_ptr(key);
        return std::make_pair(const_iterator(cur)
                            , const_iterator(cur == nullptr ? nullptr : cur->next));
    }


    // ~~ Bucket interface ~~

    size_t bucket_count() const noexcept
    {
        return d_bucket_count;
    }

    size_t max_bucket_count() const noexcept
    {
        // could potentially find a better implementation.
        // ptrdiff_t is signed
        return std::numeric_limits<ptrdiff_t>::max();
    }

    size_t bucket_size(const size_t bucket_num) const
    {
        assert(bucket_num < d_bucket_count);
        _NodeBase<_Node>* sentinel = d_buckets[bucket_num];
        if (sentinel == nullptr)
            return 0;

        _Node* cur = sentinel->next;
        size_t count = 0;
        while (cur)
        {
            if (bucket_num != bucket(cur->value.first))
                return count;
            count ++;
            cur = cur->next;
        }

        return count;
    }

    size_t bucket(const Key& key) const
    {
        return _bucket_from_hash(d_hasher(key));
    }


    // ~~ Hash policy ~~

    float load_factor() const
    {
        assert(d_bucket_count != 0);
        return d_size / float(d_bucket_count);
    }

    float max_load_factor() const
    {
        return d_max_load_factor;
    }

    void max_load_factor(float ml)
    {
        d_max_load_factor = ml;
    }

    void reserve(size_t count)
    {
        rehash(std::ceil(count / d_max_load_factor));
    }

    // Complexity is linear in the number of elements.
    void rehash(size_t count)
    {
        // If the new number of buckets makes load factor more than
        // maximum load factor (count < size() / max_load_factor()), then
        // the new number of buckets is at least size() / max_load_factor().
        d_bucket_count = std::max(count, size_t(std::ceil(d_size / d_max_load_factor)));

        if (d_size == 0)
            return;

        // Create a new unordered_map with a different bucket count and move the current
        // unordered_map's elements to it.
        typename std::remove_reference<decltype(*this)>::type other(d_bucket_count);

        _Node* cur = d_before_begin.next;
        while(cur)
        {
            // Instead of allocating new nodes and deleting old ones
            // we just modify pointers to 'create' other.
            _Node* next = cur->next;

            // 'Move' cur to other.
            const size_t bucket_num = other.bucket(cur->value.first);
            _NodeBase<_Node>* sentinel = other.d_buckets[bucket_num];
            if (sentinel == nullptr)
            {
                // 'Create' first bucket after other.d_before_begin.
                cur->next = other.d_before_begin.next;
                other.d_before_begin.next = cur;
                other.d_buckets[bucket_num] = &other.d_before_begin;

                // Update sentinel for the previous bucket which had
                // other.d_before_begin as sentinel.
                if (cur->next != nullptr)
                {
                    // next_key used to be the first after d_before_begin,
                    // now it should be the second.
                    Key next_key = cur->next->value.first;
                    other.d_buckets[other.bucket(next_key)] = cur;
                }
            }
            else
            {
                // 'Insert' at the beginning of the new bucket.
                cur->next = sentinel->next;
                sentinel->next = cur;
            }

            cur = next;
        }

        // Overwrite *this with data from new map.
        // d_size and d_max_load_factor stay the same.
        // d_bucket_count was already updated.
        d_before_begin.next = other.d_before_begin.next;
        d_buckets = other.d_buckets;

        // other.d_before_begin is part of d_buckets now, so we need
        // to find it and replace it with d_before_begin.
        // We know we have at least 1 element, so no need to check.
        size_t first_bucket = bucket(d_before_begin.next->value.first);
        d_buckets[first_bucket] = &d_before_begin;

        // Skip deallocating memory when other's destructor calls _clean()
        other.d_size = 0;
    }


    // ~~ Observers ~~

    hasher hash_function() const
    {
        return d_hasher;
    }

    key_equal key_eq() const
    {
        return d_key_equal;
    }


private:
    // ~~ Internal helpers ~~

    // Releases memory. Only operation needed from the destructor.
    void _clear() noexcept
    {
        // d_size is set to 0 in the move constructor to avoid deleting
        // nodes potentially still pointed to from the moved from map
        // (which are now also pointed to from the moved to map).
        if (d_size > 0)
        {
            _Node* cur = d_before_begin.next;
            while (cur)
            {
                _Node* to_delete = cur;
                cur = cur->next;
                delete to_delete;
            }
        }
    }

    size_t _bucket_from_hash(size_t hash) const noexcept
    {
        // could be optimized if we guarantee the number of buckets
        // is always a power of 2: hash ^ (d_bucket_count - 1).
        return hash % d_bucket_count;
    }

    _NodeBase<_Node>* _sentinel(const value_type& val) const noexcept
    {
        return d_buckets[ bucket(val.first) ];
    }

    _NodeBase<_Node>* _sentinel(size_t hash) const noexcept
    {
        return d_buckets[ _bucket_from_hash(hash) ];
    }

    // The const overload is needed because std::initializer_list
    // only has const elements.
    std::pair<iterator, bool> _insert(const value_type& val)
    {
        // Create a copy and use it as an rvalue.
        return _insert(value_type(val));
    }

    // Does not overwrite. Returns an iterator to the element with
    // val's key, and a bool representing a successful insertion.
    std::pair<iterator, bool> _insert(value_type&& val)
    {
        const size_t bucket_num = bucket(val.first);
        _NodeBase<_Node>* sentinel = d_buckets[bucket_num];

        if (sentinel == nullptr)
        {
            if (_rehash_if_needed())
                // Bucket might be different after rehash.
                return _insert(std::move(val));


            // Create a new bucket containing val and point to it
            // from d_before_begin.
            _create_bucket(std::move(val), bucket_num);
            return std::make_pair(iterator(d_before_begin.next), true);
        }

        // Return the value if it already exists.
        _Node* cur = _find_node_ptr(val.first, sentinel, bucket_num);
        if (cur)
            return std::make_pair(iterator(cur), false);

        if (_rehash_if_needed())
        {
            // Bucket might be different after rehash.
            return _insert(std::move(val));
        }

        // Insert new element in current bucket.
        cur = new _Node(sentinel->next, std::move(val));
        if (sentinel->next == d_before_begin.next)
        {
            d_before_begin.next = cur;
            d_buckets[bucket_num] = &d_before_begin;
        }
        else
            sentinel->next = cur;

        ++ d_size;

        return std::make_pair(iterator(cur), true);
    }

    // Create a bucket and point d_before_begin to it.
    void _create_bucket(value_type&& val, size_t bucket_num)
    {
        _Node* node = new _Node(d_before_begin.next, std::move(val));
        d_before_begin.next = node;
        d_buckets[bucket_num] = &d_before_begin;

        // Also update the sentinel of the bucket where d_before_begin
        // was pointing to before.
        if (node->next)
            d_buckets[bucket(node->next->value.first)] = node;

        ++ d_size;
    }

    bool _rehash_if_needed()
    {
        if ((d_size + 1) / float(d_bucket_count) > d_max_load_factor)
        {
            // STL usually uses first prime greater than 2 * bucket count.
            rehash(d_bucket_count * 2);
            return true;
        }

        return false;
    }

    iterator _erase(const Key& key)
    {
        const size_t bucket_num = bucket(key);
        _NodeBase<_Node>* prev = d_buckets[bucket_num]; // sentinel

        if (!prev)
            return iterator(nullptr);

        _Node* cur = prev->next;
        while (cur)
        {
            if (d_key_equal(cur->value.first, key))
            {
                prev->next = cur->next;
                delete cur;
                -- d_size;

                // If it was the only element in the bucket, we need to set the
                // sentinel to nullptr, in order to mark the bucket as empty.
                if (prev->next != nullptr && bucket(prev->next->value.first) != bucket_num)
                    d_buckets[bucket_num] = nullptr;

                return iterator(prev->next);
            }
            // no point searching in different buckets
            else if (bucket_num != bucket(cur->value.first))
                return iterator(nullptr);

            prev = cur;
            cur = cur->next;
        }

        // this should not happen if pos is valid
        return iterator(nullptr);
    }

    _Node* _find_node_ptr(const Key& key) const
    {
        const size_t bucket_num = bucket(key);
        _NodeBase<_Node>* sentinel = d_buckets[bucket_num];

        if (sentinel)
            return _find_node_ptr(key, sentinel, bucket_num);

        return nullptr;
    }

    // caller's responsibility to check sentinel is not null
    _Node* _find_node_ptr(const Key& key, _NodeBase<_Node>* sentinel, const size_t bucket_num) const
    {
        _Node* cur = sentinel->next;
        while (cur)
        {
            // make sure we're still in the same bucket
            if (bucket_num != bucket(cur->value.first))
                break;
            if (d_key_equal(cur->value.first, key))
                return cur;
            cur = cur->next;
        }

        return nullptr;
    }
};

} // namespace si


// ~~ Non member functions ~~

template<typename Key, typename T>
bool operator== (const si::unordered_map<Key, T>& lhs, const si::unordered_map<Key, T>& rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (auto lhs_it = lhs.cbegin(); lhs_it != lhs.cend(); ++lhs_it)
    {
        // The values can be in a different order.
        auto rhs_it = rhs.find(lhs_it->first);
        if (rhs_it == rhs.end() || rhs_it->second != lhs_it->second)
            return false;
    }

    return true;
}

template<typename Key, typename T>
bool operator!= (const si::unordered_map<Key, T>& lhs, const si::unordered_map<Key, T>& rhs)
{
    return !(lhs == rhs);
}

#endif

