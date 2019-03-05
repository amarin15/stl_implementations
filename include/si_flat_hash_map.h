#ifndef SI_FLAT_HASH_MAP_H
#define SI_FLAT_HASH_MAP_H

#include <cmath>
#include <type_traits>
#include <utility>
#include <boost/endian/conversion.hpp>


namespace si {

template<typename T
       , typename = typename std::enable_if<std::is_unsigned<T>::value>::type >
uint32_t leading_zeros(T n)
{
    int zeroes = 28;     // 32-bit
    if (sizeof(T) == 8)  // 64-bit
    {
        zeroes = 60;
        if (n >> 32)
            zeroes -= 32, n >>= 32;
    }

    if (n >> 16)
        zeroes -= 16, n >>= 16;
    if (n >> 8)
        zeroes -= 8,  n >>= 8;
    if (n >> 4)
        zeroes -= 4,  n >>= 4;

    return "\4\3\2\2\1\1\1\1\0\0\0\0\0\0\0"[n] + zeroes;
}

// Caller wants the result as an int instead of an unsigned int because
// iteration will be more efficient (no overflow check).
template<typename T
       , typename = typename std::enable_if<std::is_unsigned<T>::value>::type >
int lowest_set_bit(T n)
{
    n &= ~n + 1;
    int c = 31;          // 32-bit
    if (sizeof(T) == 8)  // 64-bit
    {
        c = 61;
        if (n & 0x00000000FFFFFFFF)
            c -= 32;
    }

    if (n & 0x0000FFFF0000FFFF)
        c -= 16;
    if (n & 0x00FF00FF00FF00FF)
        c -= 8;
    if (n & 0x0F0F0F0F0F0F0F0F)
        c -= 4;
    if (n & 0x3333333333333333)
        c -= 2;
    if (n & 0x5555555555555555)
        c -= 1;

    return c;
}


using ctrl_t = signed char;
using h2_t   = uint8_t;

enum Ctrl : ctrl_t
{
    kEmpty    = -128, // 0b10000000, 0x80
    kDeleted  = -2,   // 0b11111110, 0xFE
    kSentinel = -1,   // 0b11111111, 0xFF
//  kFull                0b0xxxxxxx
};

static_assert(kEmpty & kDeleted & kSentinel & 0x80,
    "Special markers need to have the MSB to make checking for them efficient");
static_assert(kEmpty < kDeleted && kDeleted < kSentinel,
    "Iterators assume ctrl bytes are empty or deleted when they are <= kDeleted.");
static_assert(~kEmpty & ~kDeleted & kSentinel & 0x7F,
              "kEmpty and kDeleted must share an unset bit that is not shared "
              "by kSentinel to make the scalar test for matchEmptyOrDeleted() "
              "efficient");

inline bool is_empty(ctrl_t ctrl)
{
    return ctrl == kEmpty;
}

inline bool is_deleted(ctrl_t ctrl)
{
    return ctrl == kDeleted;
}

inline bool is_empty_or_deleted(ctrl_t ctrl)
{
    return ctrl <= kEmpty;
}

inline bool is_full(ctrl_t ctrl)
{
    return ctrl >= 0;
}


// Returns a hash seed.
//
// The seed consists of the ctrl pointer, which adds enough entropy to
// ensure non-determinism of iteration order in most cases.
inline size_t hash_seed(const ctrl_t* ctrl)
{
    // The low bits of the pointer have little or no entropy because of
    // alignment. We shift the pointer to try to use higher entropy bits. A
    // good number seems to be 12 bits, because that aligns with page size.
    return reinterpret_cast<uintptr_t>(ctrl) >> 12;
}

inline size_t H1(size_t hash, const ctrl_t* ctrl)
{
    return hash >> 7 ^ hash_seed(ctrl);
}

inline ctrl_t H2(size_t hash)
{
    return hash & 0x7F; // 0b01111111
}


// Groups without empty slots (but maybe with deleted slots) extend the probe
// sequence. The probing algorithm is quadratic. Given N the number of groups,
// the probing function for the i-th probe is:
//
//   P(0) = H1 % N
//
//   P(i) = (P(i - 1) + i) % N
//
// This probing function guarantees that after N probes, all the groups of the
// table will be probed exactly once.
template <size_t Width>
class ProbeSeq
{
public:
    ProbeSeq(size_t hash, size_t mask)
    {
        // Intended to be used with flat_hash_map's capacity as a mask,
        // which is guaranteed to be a power of 2 minus 1.
        // Example: for N = 4 groups with a Width of 8, we would use
        // a capacity of 31 = 0b00011111.
        assert(((mask + 1) & mask) == 0 && "not a mask");
        d_mask = mask;
        d_offset = hash & d_mask; // P(0) - this can be any offset in any group.
    }

    size_t offset() const
    {
        return d_offset;
    }

    size_t offset(size_t i) const
    {
        return (d_offset + i) & d_mask;
    }

    void next()
    {
        d_index  += Width;   // i-th probe index
        d_offset += d_index; // P(i - 1) + i
        d_offset &= d_mask;  // P(i)
    }

    size_t index() const
    {
        return d_index;
    }

private:
    size_t d_mask;      // a valid power of 2 minus 1 (capacity)
    size_t d_offset;    // points to the start of the current group
    size_t d_index = 0; // 0-based probe index
};


// Provides an easy way to iterate through the set bits of given mask:
// for (int i : BitMask<uint64_t>(0x0000000080800000)) -> yields 2, 3.
//
// The above iteration works as follows:
// - create an iterator with the value of begin()  -- a BitMask object
// with the initial value
// - while the current iterator is != end()  -- non-zero
//     - dereference the iterator (BitMask) and return the lowest set bit
//     - call operator++()  -- unset the lowest set bit.
template<typename T
       , typename = typename std::enable_if<std::is_unsigned<T>::value>::type >
class BitMask
{
public:
    explicit BitMask(T mask)
        : d_mask(mask)
    {}

    BitMask begin() const
    {
        return *this;
    }

    BitMask end() const
    {
        return BitMask(0);
    }

    BitMask& operator++()
    {
        // Unset lowest set bit
        d_mask &= (d_mask - 1);
        return *this;
    }

    // Iterators are also BitMask objects and when we dereference them we
    // want to get the lowest set bit.
    // Returning a signed int is more efficient than an unsigned int because
    // the compiler won't need to check for overflows when iterating.
    int operator*() const
    {
        return lowestSetBit();
    }

    int lowestSetBit() const
    {
        return lowest_set_bit(d_mask) >> 3;
    }

    // Used to check if the Group match results are empty.
    explicit operator bool() const
    {
        return d_mask != 0;
    }

    // Used in erase
    int leadingZeros() const
    {
        return leading_zeros(d_mask);
    }

private:
    friend bool operator==(const BitMask& a, const BitMask& b)
    {
        return a.d_mask == b.d_mask;
    }

    friend bool operator!=(const BitMask& a, const BitMask& b)
    {
        return a.d_mask != b.d_mask;
    }

    T d_mask;
};


// Logical group created from the control bytes in the flat_hash_map.
// It's used to match a 1 byte hash against multiple control bytes
// at the same time.
struct Group
{
    uint64_t            ctrl;      // 8 bytes
    static const size_t width = 8; // 8 / sizeof(ctrl_t)

    explicit Group(const ctrl_t* pos)
        // Load as little endian. This allows us to go through the d_ctrl
        // bytes in order using the lowest set bit in the Bitmask.
        : ctrl(boost::endian::native_to_little(*(uint64_t*)(pos)))
    {}

    BitMask<uint64_t> match(h2_t hash) const
    {
        // Full slots have the high bit set to 0.
        // ~hash will have the high bit equal to 1 if hash is full.
        // 0x80 is 0b10000000.
        // ~hash & 0x80 will be 0x80 if hash is full, otherwise 0.
        //
        // We want to match 8 (width) ctrl bytes at the same type
        // against the same 1 byte hash from H2, so we create hash_x8
        // which has each of its 8 bytes equal to hash.
        //
        // The BitMask allows us to iterate efficiently through all
        // set bits (1 for each 0x80 byte in this case).
        constexpr uint64_t msbs = 0x8080808080808080ULL;
        constexpr uint64_t lsbs = 0x0101010101010101ULL;

        auto x = ctrl ^ (lsbs * hash);
        return BitMask<uint64_t>((x - lsbs) & ~x & msbs);
    }

    BitMask<uint64_t> matchEmpty() const
    {
        constexpr uint64_t msbs = 0x8080808080808080ULL;
        return BitMask<uint64_t>((ctrl & (~ctrl << 6)) & msbs);
    }

    BitMask<uint64_t> matchEmptyOrDeleted() const
    {
        constexpr uint64_t msbs = 0x8080808080808080ULL;
        // If the last byte in ctrl is kSentinel (0b11111111),
        // ~ctrl << 7 will have the last 8 bits 0, so won't match.
        // If the last byte is kEmpty (0b10000000) or kDeleted
        // (0b11111110), then the last 8 bits will be 0b10000000
        // which is equal to 0x80, so will match.
        return BitMask<uint64_t>((ctrl & (~ctrl << 7)) & msbs);
    }

    uint32_t countLeadingEmptyOrDeleted() const
    {
        // 0xFE = 0b11111110
        // Lowest set bit is equivalent to the number of trailing zeros.
        constexpr uint64_t gaps = 0x00FEFEFEFEFEFEFEULL;
        return (lowest_set_bit(((~ctrl & (ctrl >> 7)) | gaps) + 1) + 7) >> 3;
    }

    // Applies mapping for every byte in ctrl:
    //   DELETED -> EMPTY
    //   EMPTY   -> EMPTY
    //   FULL    -> DELETED
    void convertSpecialToEmptyAndFullToDeleted(ctrl_t* dst)
    {
        constexpr uint64_t msbs = 0x8080808080808080ULL;
        constexpr uint64_t lsbs = 0x0101010101010101ULL;
        auto x = ctrl & msbs;
        auto res = (~x + (x >> 7)) & ~lsbs;

        *(uint64_t*)(dst) = boost::endian::native_to_little(res);
    }
};


template<
    typename Key
  , typename T
  , typename Hash = std::hash<Key>
  , typename KeyEqual = std::equal_to<Key>
> class flat_hash_map
{
    // std::is_invocable is only present in C++17, everything else compiles
    // with C++11. You can find a C+11 implementation of is_invocable here:
    // https://github.com/gcc-mirror/gcc/blob/d3a3029ca7489cb168d493de3d695809e84ffb0f/libstdc%2B%2B-v3/include/std/type_traits#L2661
    static_assert(std::is_invocable<Hash, Key>::value
                 , "Hash function must be invocable with a Key type");

    // Forward declarations
    struct _Iterator;
    struct _ConstIterator;

public:
    // ~~ Types ~~

    using key_type        = Key;
    using mapped_type     = T;
    using value_type      = std::pair<const Key, T>;
    using difference_type = std::ptrdiff_t;
    using hasher          = Hash;
    using key_equal       = KeyEqual;
    using iterator        = _Iterator;
    using const_iterator  = _ConstIterator;


private:
    // ~~ Types ~~

    using slot_t = std::pair<Key, T>;


    // ~~ Internal classes ~~

    class _IteratorBase
    {
        friend class flat_hash_map;

    public:
        // LegacyForwardIterator
        using iterator_category = std::forward_iterator_tag;
        using value_type        = flat_hash_map::slot_t;
        using difference_type   = typename flat_hash_map::difference_type;

        ctrl_t* ctrl_p = nullptr;

        // Wrap slot_p in an anonymous union to avoid uninitialized warnings.
        // The member is not initialized on end iterators.
        union
        {
            value_type* slot_p;
        };

    protected:
        _IteratorBase(ctrl_t* cp) noexcept // for end()
            : ctrl_p(cp)
        {}

        _IteratorBase(ctrl_t* cp, slot_t* sp) noexcept
            : ctrl_p(cp)
            , slot_p(sp)
        {}

        // Caller's job to check this is not called on end()
        void increment()
        {
            ++ctrl_p;
            ++slot_p;
            _skip_empty_or_deleted();
        }

    private:
        void _skip_empty_or_deleted()
        {
            while (is_empty_or_deleted(*ctrl_p))
            {
                // ctrl_p is not necessarily aligned to Group::width.
                uint32_t shift = Group{ctrl_p}.countLeadingEmptyOrDeleted();
                ctrl_p += shift;
                slot_p += shift;
            }
        }

    public:
        bool operator==(const _IteratorBase& other) const noexcept
        {
            // Compare pointers, not values.
            return ctrl_p == other.ctrl_p;
        }

        bool operator!=(const _IteratorBase& other) const noexcept
        {
            return ctrl_p != other.ctrl_p;
        }
    };

    struct _Iterator : public _IteratorBase
    {
        using pointer   = slot_t*;
        using reference = slot_t&;

        explicit _Iterator(ctrl_t* cp) noexcept // for end()
            : _IteratorBase(cp)
        {}

        explicit _Iterator(ctrl_t* cp, slot_t* sp) noexcept
            : _IteratorBase(cp, sp)
        {}

        // Caller's job to check operators are not called on end()
        reference operator*() const noexcept
        {
            //static_assert( std::is_same<typename _IteratorBase::value_type, slot_t>::value );
            return *(this->slot_p);
        }

        pointer operator->() const noexcept
        {
            return this->slot_p;
        }

        // Prefix operator++.
        _Iterator& operator++() noexcept
        {
            this->increment();
            return *this;
        }

        // Postfix operator++.
        _Iterator operator++(int) noexcept
        {
            auto it = *this;
            ++ *this;
            return it;
        }
    };

    struct _ConstIterator : public _IteratorBase
    {
        using pointer   = const slot_t*;
        using reference = const slot_t&;

        explicit _ConstIterator(ctrl_t* cp) noexcept // for end()
            : _IteratorBase(cp)
        {}

        explicit _ConstIterator(ctrl_t* cp, slot_t* sp) noexcept
            : _IteratorBase(cp, sp)
        {}

        // Implicit construction from iterator.
        _ConstIterator(const _Iterator& i)
            : _IteratorBase(i.ctrl_p, i.slot_p)
        {}

        // Caller's job to check operators are not called on end()
        reference operator*() const noexcept
        {
            return *this->slot_p;
        }

        pointer operator->() const noexcept
        {
            return this->slot_p;
        }

        // Prefix operator++.
        _ConstIterator& operator++() noexcept
        {
            this->increment();
            return *this;
        }

        // Postfix operator++.
        _ConstIterator operator++(int) noexcept
        {
            auto it = *this;
            ++ *this;
            return it;
        }
    };


    // ~~ Data ~~

    // Total number of slots, guaranteed to be a power of 2 minus 1.
    // Equivalent to bucket count (each bucket only holds 1 slot).
    size_t              d_capacity;
    // Number of full slots.
    size_t              d_size;
    // The max load factor is 87.5%, after which the table doubles in size
    // (making load factor go down by 2x). Thus size() is usually between
    // 0.4375 * bucket_count() and 0.875 * bucket_count().
    float               d_max_load_factor = 0.875;
    // When we delete an element, we only remove it from the metadata and
    // decrease d_size, but we don't delete the slot. d_growth_left tells
    // us when we hit the max load factor, so we can either rehash or remove
    // elements marked as deleted.
    size_t              d_growth_left;
    // The table stores elements inline in a slot array. In addition to the slot
    // array the table maintains some control state per slot. The extra state is
    // one byte per slot and stores empty or deleted marks (with high bit 1), or
    // alternatively 7 bits from the hash of an occupied slot.
    // The table is split into logical groups of slots, like so:
    //
    //      Group 1         Group 2        Group 3
    // +---------------+---------------+---------------+
    // | | | | | | | | | | | | | | | | | | | | | | | | |
    // +---------------+---------------+---------------+
    //
    // Uses O((sizeof(std::pair<const K, V>) + 1) * bucket_count()) bytes.
    std::vector<ctrl_t> d_ctrl;
    std::vector<slot_t> d_slots;

    hasher              d_hasher;
    key_equal           d_key_equal;

public:
    // ~~ Constructors ~~

    // (1) default constructor
    // No allocation for the table's elements is made.
    explicit flat_hash_map(size_t bucket_count = 0)
      : d_capacity(_normalize_capacity(bucket_count))
      , d_size(0)
      , d_growth_left(_growth_left_from_size())
      , d_ctrl(_ctrl_from_capacity())
      , d_slots(d_capacity)
    {}

    // (2) range constructor
    template <typename InputIt>
    flat_hash_map(InputIt first
          , InputIt last
          , size_t bucket_count = 0
          , const Hash& hash = Hash()
          , const key_equal& key_eq = key_equal()
    ) : d_capacity(_normalize_capacity(bucket_count))
      , d_size(0)
      , d_growth_left(_growth_left_from_size())
      , d_ctrl(_ctrl_from_capacity())
      , d_slots(d_capacity)
      , d_hasher(hash)
      , d_key_equal(key_eq)
    {
        insert(first, last);
    }

    // (3) copy constructor
    // No pointers involved, so we can copy everything.
    flat_hash_map(const flat_hash_map& other)
      : d_capacity(other.d_capacity)
      , d_size(other.d_size)
      , d_max_load_factor(other.d_max_load_factor)
      , d_growth_left(other.d_growth_left)
      , d_ctrl(_ctrl_from_capacity())
      , d_slots(d_capacity)
      , d_hasher(other.d_hasher)
      , d_key_equal(other.d_key_equal)
    {
        _insert_in_empty_map(other);
    }

    // (4) move constructor
    flat_hash_map(flat_hash_map&& other)
      : d_capacity(std::move(other.d_capacity))
      , d_size(std::move(other.d_size))
      , d_max_load_factor(std::move(other.d_max_load_factor))
      , d_growth_left(std::move(other.d_growth_left))
      , d_ctrl(std::move(other.d_ctrl))
      , d_slots(std::move(other.d_slots))
      , d_hasher(std::move(other.d_hasher))
      , d_key_equal(std::move(other.d_key_equal))
    {}

    // (5) initializer list
    flat_hash_map(const std::initializer_list<value_type>& init
          , size_t bucket_count = 0
    ) : d_capacity(_normalize_capacity(bucket_count))
      , d_size(0)
      , d_growth_left(_growth_left_from_size())
      , d_ctrl(_ctrl_from_capacity())
      , d_slots(d_capacity)
    {
        insert(init);
    }

    ~flat_hash_map() = default;


    flat_hash_map<Key, T>& operator=(const flat_hash_map<Key, T>& other)
    {
        d_capacity        = other.d_capacity;
        d_size            = other.d_size;
        d_max_load_factor = other.d_max_load_factor;
        d_growth_left     = other.d_growth_left;
        d_hasher          = other.d_hasher;
        d_key_equal       = other.d_key_equal;
        d_ctrl            = _ctrl_from_capacity();
        d_slots.clear();
        d_slots.reserve(d_capacity);

        _insert_in_empty_map(other);

        return *this;
    }

    flat_hash_map<Key, T>& operator=(flat_hash_map<Key, T>&& other)
    {
        d_capacity        = std::move(other.d_capacity);
        d_size            = std::move(other.d_size);
        d_max_load_factor = std::move(other.d_max_load_factor);
        d_growth_left     = std::move(other.d_growth_left);
        d_hasher          = std::move(other.d_hasher);
        d_key_equal       = std::move(other.d_key_equal);
        d_ctrl            = std::move(other.d_ctrl);
        d_slots           = std::move(other.d_slots);

        return *this;
    }


    // ~~ Iterators ~~

    iterator begin() noexcept
    {
        auto it = _iterator_at(0);
        it._skip_empty_or_deleted();
        return it;
    }

    iterator end() noexcept
    {
        return iterator(d_ctrl.data() + d_capacity);
    }

    const_iterator begin() const noexcept
    {
        return const_cast<flat_hash_map*>(this)->begin();
    }

    const_iterator end() const noexcept
    {
        return const_cast<flat_hash_map*>(this)->end();
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
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
        // Could potentially find a better implementation.
        // ptrdiff_t is signed.
        return std::numeric_limits<ptrdiff_t>::max();
    }


    // ~~ Modifiers ~~

    void clear() noexcept
    {
        d_ctrl = _ctrl_from_capacity();
        d_slots.clear();
        d_size = 0;
        d_growth_left = _growth_left_from_size();
    }

    // (1) copy value
    std::pair<iterator, bool> insert(const slot_t& val)
    {
        return _insert(val);
    }

    // (1) move value
    std::pair<iterator, bool> insert(slot_t&& val)
    {
        return _insert(std::move(val));
    }

    // (2) emplace value
    template < typename P
             , typename NotUsed = typename std::enable_if<
                // Only allow forwarding references that slot_t can be
                // constructed from.
                std::is_constructible<slot_t, P&&>::value
                // Overload resolution makes sure that (1) takes priority over
                // (2) when P is slot_t, because it matches it more closely,
                // which means we don't make an extra copy calling emplace.
                // That makes the below condition redundant:
                // && !std::is_same<P, slot_t>::value
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
        // Construct slot_t from both lvalues or rvalues
        // and call _insert with an rvalue.
        return _insert(slot_t(std::forward<Args>(args)...));
    }


    // On erase a slot is cleared. In case the group did not have any empty
    // slots before the erase, the erased slot is marked as deleted.
    // The iterator pos must be valid and dereferenceable.
    //
    // Erases the element pointed to by `it`.  Unlike `std::unordered_set::erase`,
    // this method returns void to reduce algorithmic complexity to O(1).  In
    // order to erase while iterating across a map, use the following idiom (which
    // also works for standard containers):
    //
    // for (auto it = m.begin(), end = m.end(); it != end;) {
    //   if (<pred>) {
    //     m.erase(it++);
    //   } else {
    //     ++it;
    //   }
    // }
    void erase(const_iterator pos)
    {
        assert(pos != end());
        if (is_full(*pos.ctrl_p))
            _erase_meta_only(pos);
    }

    size_t erase(const key_type& key)
    {
        auto it = find(key);
        if (it == end())
            return 0;
        erase(it);
        return 1;
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        while (first != last)
            erase(first++);

        return iterator(last.ctrl_p, last.slot_p);
    }


    void swap(flat_hash_map& other) noexcept
    {
        std::swap(d_ctrl, other.d_ctrl);
        std::swap(d_slots, other.d_slots);
        std::swap(d_size, other.d_size);
        std::swap(d_capacity, other.d_capacity);
        std::swap(d_growth_left, other.d_growth_left);
        std::swap(d_hasher, other.d_hasher);
        std::swap(d_key_equal, other.d_key_equal);
    }


    // ~~ Lookup ~~

    // On lookup the hash is split into two parts:
    // - H2: 7 bits (those stored in the control bytes)
    // - H1: the rest of the bits
    // The groups are probed using H1. For each group the slots are matched to H2 in
    // parallel. Because H2 is 7 bits (128 states) and the number of slots per group
    // is low (8 or 16) in almost all cases a match in H2 is also a lookup hit.
    const T& at(const Key& key) const
    {
        auto it = find(key);
        if (it == end())
            throw std::out_of_range("Key not found.");
        return it->second;
    }

    // Use const implementation and remove constness.
    T& at(const Key& key)
    {
        return const_cast<T&>(static_cast<const flat_hash_map<T, Key>*>(this)->at(key));
    }

    T& operator[](Key&& key)
    {
        auto p = _insert(slot_t(std::move(key), T()));
        return p.first->second;
    }

    T& operator[](const Key& key)
    {
        return this->operator[](Key(key));
    }

    size_t count(const Key& key) const
    {
        return find(key) != end();
    }

    iterator find(const Key& key)
    {
        const size_t hash = d_hasher(key);
        auto seq = _probe(hash);

        while (true)
        {
            Group g(d_ctrl.data() + seq.offset());
            for (int i : g.match(H2(hash)))
            {
                size_t offset = seq.offset(i);
                if (d_key_equal(key, d_slots[offset].first))
                    return _iterator_at(offset);
            }

            // Check if at least 1 slot in the group is empty. If we have deleted slots,
            // but we don't have empty slots, then we can't stop our search because our
            // key might exist in one of the next groups in the probing sequence.
            if (g.matchEmpty())
                return end();

            seq.next();
        }
    }

    const_iterator find(const Key& key) const
    {
        return const_cast<flat_hash_map*>(this)->find(key);
    }

    std::pair<iterator, iterator> equal_range(const Key& key)
    {
        iterator it = find(key);
        if (it != end())
            return {it, std::next(it)};
        return {it, it};
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
    {
        const_iterator it = find(key);
        if (it != cend())
            return {it, std::next(it)};
        return {it, it};
    }

    bool contains(const Key& key) const
    {
        return find(key) != end();
    }


    // ~~ Bucket interface ~~

    size_t bucket_count() const noexcept
    {
        return d_capacity;
    }

    size_t capacity() const noexcept
    {
        return d_capacity;
    }


    // ~~ Hash policy ~~

    float load_factor() const
    {
        assert(d_capacity != 0);
        return d_size / float(d_capacity);
    }

    float max_load_factor() const
    {
        return d_max_load_factor;
    }

    void max_load_factor(float ml)
    {
        d_max_load_factor = ml;
    }

    void reserve(size_t new_capacity)
    {
        rehash(std::ceil(new_capacity / d_max_load_factor));
    }

    // Complexity is linear in the number of elements.
    void rehash(size_t new_capacity)
    {
        new_capacity = std::max(new_capacity, size_t(std::ceil(d_size / d_max_load_factor)));
        _resize(_normalize_capacity(new_capacity));
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

    // Rounds up the capacity to the next power of 2 minus 1 and ensures it is
    // greater or equal to Group::width - 1.
    size_t _normalize_capacity(size_t n) const
    {
        constexpr size_t min_capacity = Group::width - 1;
        return n <= min_capacity
            ? min_capacity
            : std::numeric_limits<size_t>::max() >> leading_zeros(n);
    }

    bool _is_valid_capacity(size_t n) const
    {
        return ((n + 1) & n) == 0 && n >= Group::width - 1;
    }

    size_t _growth_left_from_size() const
    {
        return std::floor(d_capacity * d_max_load_factor) - d_size;
    }

    void _reset_growth_left()
    {
        d_growth_left = _growth_left_from_size();
    }

    std::vector<ctrl_t> _ctrl_from_capacity() const
    {
        // Also eserve space for a sentinel and an extra group so we know
        // when to stop with searches.
        std::vector<ctrl_t> ctrl(d_capacity + 1 + Group::width, kEmpty);
        ctrl[d_capacity] = kSentinel;

        return ctrl;
    }

    ProbeSeq<Group::width> _probe(size_t hash) const
    {
        return ProbeSeq<Group::width>(H1(hash, d_ctrl.data()), d_capacity);
    }

    // Sets the control byte, and if i < Group::width, set the cloned byte at
    // the end too.
    void _set_ctrl(size_t i, ctrl_t ctrl)
    {
        assert(i < d_capacity);

        d_ctrl[i] = ctrl;
        d_ctrl[((i - Group::width) & d_capacity) + Group::width] = ctrl;
    }

    iterator _iterator_at(size_t pos) const
    {
        return iterator((ctrl_t*)d_ctrl.data() + pos, (slot_t*)d_slots.data() + pos);
    }

    void _insert_in_empty_map(const flat_hash_map& other)
    {
        // Instead of calling insert, we can do something faster,
        // because the table is guaranteed to be empty.
        for (const auto& v : other)
        {
            const size_t hash = d_hasher(v.first);
            size_t target_offset = _find_first_non_full(hash);
            _set_ctrl(target_offset, H2(hash));
            d_slots[target_offset] = v;
        }
    }

    // The const overload is needed because std::initializer_list
    // only has const elements.
    std::pair<iterator, bool> _insert(const slot_t& val)
    {
        // Create a copy and use it as an rvalue.
        return _insert(slot_t(val));
    }

    // Does not overwrite. Returns an iterator to the element with
    // val's key, and a bool representing a successful insertion.
    //
    // Once the right group is found, its slots are filled in order.
    std::pair<iterator, bool> _insert(slot_t&& val)
    {
        const size_t hash = d_hasher(val.first);
        auto seq = _probe(hash);

        while (true)
        {
            Group g(d_ctrl.data() + seq.offset());
            for (int i : g.match(H2(hash)))
            {
                size_t offset = seq.offset(i);
                if (d_key_equal(val.first, d_slots[offset].first)) // already exists
                    return {_iterator_at(offset), false};
            }

            // Check if at least 1 slot in the group is empty. If we have deleted slots,
            // but we don't have empty slots, then we can't stop our search because our
            // key might exist in one of the next groups in the probing sequence.
            if (g.matchEmpty())
            {
                // At this point we know our key does not exist in the map, so we can
                // insert it in the first empty or deleted slot.
                size_t insert_pos = _prepare_insert(hash, val.first);

                d_slots[insert_pos] = std::move(val);
                return {_iterator_at(insert_pos), true};
            }

            seq.next();
        }
    }

    // Resizes if necessary, returns position of first non full slot,
    // and marks it as FULL in the metadata.
    size_t _prepare_insert(size_t hash, const Key& key)
    {
        size_t target_offset = _find_first_non_full(hash);

        if (d_growth_left == 0 && !is_deleted(d_ctrl[target_offset]))
        {
            _rehash_and_grow_if_necessary();

            // hash needs to be recomputed because the capacity might have changed.
            hash = d_hasher(key);
            target_offset = _find_first_non_full(hash);
        }

        ++ d_size;
        d_growth_left -= is_empty(d_ctrl[target_offset]);

        // Mark slot as FULL.
        _set_ctrl(target_offset, H2(hash));
        return target_offset;
    }

    // Probes the map with the probe sequence for hash and returns
    // the offset for the first empty or deleted slot.
    size_t _find_first_non_full(size_t hash, ctrl_t* ctrl) const
    {
        auto seq = _probe(hash);
        while (true)
        {
            Group g{ctrl + seq.offset()};
            auto mask = g.matchEmptyOrDeleted();
            if (mask)
                return seq.offset(mask.lowestSetBit());

            // Probing index is greater than the capacity only after we
            // visited all the groups.
            assert(seq.index() < d_capacity && "table is full!");
            seq.next();
        }
    }

    size_t _find_first_non_full(size_t hash) const
    {
        return _find_first_non_full(hash, (ctrl_t*)d_ctrl.data());
    }

    void _rehash_and_grow_if_necessary()
    {
        if (d_capacity == 0)
        {
            _resize(Group::width - 1);
        }
        else if (d_size <= d_capacity * d_max_load_factor / 2)
        {
            // Squash DELETED without growing if there is enough capacity.
            _drop_deletes_without_resize();
        }
        else
        {
            // Otherwise grow the container.
            _resize(d_capacity * 2 + 1);
        }
    }

    void _resize(size_t new_capacity)
    {
        assert(_is_valid_capacity(new_capacity));

        size_t old_capacity = d_capacity;
        d_capacity = new_capacity;
        std::vector<ctrl_t> new_ctrl(_ctrl_from_capacity());
        std::vector<slot_t> new_slots(new_capacity);
        d_ctrl.swap(new_ctrl);
        d_slots.swap(new_slots);
        _reset_growth_left();

        for (size_t i = 0; i != old_capacity; ++i)
        {
            if (is_full(new_ctrl[i]))
            {
                size_t hash = d_hasher(new_slots[i].first);
                size_t new_i = _find_first_non_full(hash, d_ctrl.data());

                // Mark as FULL in new metadata.
                _set_ctrl(new_i, H2(hash)); // recompute hash with new seed
                d_slots[new_i] = std::move(new_slots[i]);
            }
        }
    }

    // Algorithm:
    // - mark all DELETED slots as EMPTY
    // - mark all FULL slots as DELETED
    // - for each slot marked as DELETED
    //     hash = Hash(element)
    //     target = find_first_non_full(hash)
    //     if target is in the same group
    //       mark slot as FULL
    //     else if target is EMPTY
    //       transfer element to target
    //       mark slot as EMPTY
    //       mark target as FULL
    //     else if target is DELETED
    //       swap current element with target element
    //       mark target as FULL
    //       repeat procedure for current slot with moved from element (target)
    void _drop_deletes_without_resize()
    {
        _convert_deleted_to_empty_and_full_to_deleted();

        // Uninitialized storage with size at most sizeof(slot_t) and whose
        // alignment requirement is a divisor of alignof(slot_t).
        typename std::aligned_storage<sizeof(slot_t), alignof(slot_t)>::type raw;
        slot_t* slot = reinterpret_cast<slot_t*>(&raw);

        for (size_t i = 0; i != d_capacity; ++i)
        {
            if (!is_deleted(d_ctrl[i]))
                continue;

            // Find the offset of the first non full slot in the probe sequence
            // of the current slot's key.
            size_t hash = d_hasher(d_slots[i].first);
            size_t new_i = _find_first_non_full(hash);

            // Verify if the old and new i fall within the same group wrt the hash.
            // If they do, we don't need to move the object as it falls already in
            // the best probe.
            auto hash_offset = _probe(hash).offset();
            const auto probe_index = [&](size_t pos)
            {
                return ((pos - hash_offset) & d_capacity) / Group::width;
            };

            if (probe_index(new_i) == probe_index(i))
            {
                // Element doesn't move. Mark slot as FULL.
                _set_ctrl(i, H2(hash));
            }
            else if (is_empty(d_ctrl[new_i]))
            {
                // Transfer element to the empty spot.
                d_slots[new_i] = std::move(d_slots[i]);

                // Mark slot as EMPTY.
                _set_ctrl(i, kEmpty);

                // Mark target as FULL.
                _set_ctrl(new_i, H2(hash));
            }
            else
            {
                // Comes from find_first_non_full and it's not empty.
                assert(is_deleted(d_ctrl[new_i]));

                // Swap current element with target element.
                *slot = d_slots[i];
                d_slots[i] = d_slots[new_i];
                d_slots[new_i] = *slot;

                // Mark target as FULL.
                _set_ctrl(new_i, H2(hash));

                // Repeat procedure for current slot with moved from element.
                --i;
            }
        }

        _reset_growth_left();
    }

    void _convert_deleted_to_empty_and_full_to_deleted()
    {
        assert(d_ctrl[d_capacity] == kSentinel);
        ctrl_t* start = d_ctrl.data();
        for (ctrl_t* pos = start; pos != start + d_capacity + 1; pos += Group::width)
            Group{pos}.convertSpecialToEmptyAndFullToDeleted(pos);

        // Copy the cloned ctrl bytes.
        std::memcpy(start + d_capacity + 1, start, Group::width);

        d_ctrl[d_capacity] = kSentinel;
    }

    // Updates the metadata in d_ctrl, without altering d_slots.
    void _erase_meta_only(const_iterator it)
    {
        assert(is_full(*it.ctrl_p) && "erasing a dangling iterator");
        -- d_size;
        const size_t index = it.ctrl_p - d_ctrl.data();
        const size_t index_before = (index - Group::width) & d_capacity;
        const auto empty_after = Group(it.ctrl_p).matchEmpty();
        const auto empty_before = Group(d_ctrl.data() + index_before).matchEmpty();

        // We count how many consecutive non empties we have to the right and to
        // the left of `it`. If the sum is >= Group::width then there is at least
        // one probe window that might have seen a full group.
        bool was_never_full =
            empty_before && empty_after &&
            static_cast<size_t>(empty_after.lowestSetBit() + // trailing zeros
                                empty_before.leadingZeros()) < Group::width;

        _set_ctrl(index, was_never_full ? kEmpty : kDeleted);
        d_growth_left += was_never_full;
    }
};


// ~~ Non member functions ~~

template<typename Key, typename T>
bool operator== (const si::flat_hash_map<Key, T>& lhs, const si::flat_hash_map<Key, T>& rhs)
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
bool operator!= (const si::flat_hash_map<Key, T>& lhs, const si::flat_hash_map<Key, T>& rhs)
{
    return !(lhs == rhs);
}


} // namespace si

#endif

