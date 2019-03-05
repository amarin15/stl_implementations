#include <gtest/gtest.h>

#include <string>
#include <vector>
#include <utility>

#include <unordered_map>
#include <si_unordered_map.h>
#include <si_flat_hash_map.h>


// Constructors, Capacity

template <template<typename...> class MapType >
void test_integral_keys()
{
    MapType<unsigned int, int> m1;
    EXPECT_TRUE(m1.empty());

    MapType<int, float> m2(15);
    EXPECT_TRUE(m2.empty());
    EXPECT_TRUE(m2.bucket_count() >= 15);
}

template <template<typename...> class MapType >
void test_string_keys()
{
    MapType<std::string, double> empty;
}

struct HashableKey
{
    unsigned value;
};

struct HashableKeyHash {
    // don't forget the const at the end
    size_t operator()(const HashableKey& hk) const
    {
        return std::hash<unsigned>{}(hk.value);
    }
};

template <template<typename...> class MapType>
void test_custom_hashable_keys()
{
    MapType<HashableKey, float, HashableKeyHash> empty;
}

class NonHashableKey
{};

template <template<typename...> class MapType>
void fail_custom_non_hashable_keys()
{
    // uncomment to confirm this will not compile
    // MapType<NonHashableKey, float> empty;
}

template <template<typename...> class MapType>
void test_from_range()
{
    std::vector<std::pair<int, int>> v(5, std::make_pair(2, 3));
    v[1].first  = 7;
    v[2].second = 8;

    MapType<unsigned int, int> m(v.begin(), v.end() - 1);
    EXPECT_TRUE(m.size() == 2); // 4 out of 5 have equal keys
    EXPECT_TRUE(m[7] == 3);     // v[1] is (7,3)
    EXPECT_TRUE(m[2] == 3);     // v[4] is (2,3)
    EXPECT_TRUE(m.bucket_count() >= m.size());
    EXPECT_TRUE(m.load_factor() <= m.max_load_factor());
}

template <template<typename...> class MapType>
void test_copy_constructor()
{
    MapType<unsigned int, int> m1;
    int n = 10;
    for (int i = 0; i < n; i ++)
        m1[i] = i * 10;

    MapType<unsigned int, int> m2(m1);

    EXPECT_EQ(m2.size(), m1.size());
    EXPECT_EQ(m2.size(), n);
    EXPECT_EQ(m2.max_size(), m1.max_size());
    EXPECT_EQ(m2.bucket_count(), m1.bucket_count());
    for (int i = 0; i < n; i ++)
    {
        EXPECT_EQ(m2[i], i * 10);
        EXPECT_EQ(m2[i], m1[i]);
    }
}

template <template<typename...> class MapType>
void test_move_constructor()
{
    MapType<unsigned int, int> m1;
    int n = 10;
    for (int i = 0; i < n; i ++)
        m1[i] = i * 10;
    size_t bucket_count = m1.bucket_count();

    MapType<unsigned int, int> m2(std::move(m1));

    EXPECT_EQ(m2.size(), n);
    EXPECT_EQ(m2.bucket_count(), bucket_count);
    for (int i = 0; i < n; i ++)
        EXPECT_EQ(m2[i], i * 10);
}

template <template<typename...> class MapType>
void test_from_initializer_list()
{
    MapType<unsigned int, int> m { {1, 10}, {1, 100}, {2, 20}, {3, 30} };
    EXPECT_TRUE(m.size() == 3);
    EXPECT_TRUE(m[1] == 10);
    EXPECT_TRUE(m[2] == 20);
    EXPECT_TRUE(m[3] == 30);
    EXPECT_TRUE(m.bucket_count() >= m.size());
}

template <template<typename...> class MapType>
void test_constructors()
{
    test_integral_keys<MapType>();         // (1)
    test_string_keys<MapType>();

    test_custom_hashable_keys<MapType>();
    fail_custom_non_hashable_keys<MapType>();

    test_from_range<MapType>();            // (2)

    test_copy_constructor<MapType>();      // (3)
    test_move_constructor<MapType>();      // (4)

    test_from_initializer_list<MapType>(); // (5)
}

template <template<typename...> class MapType>
void test_assignment_operator()
{
    MapType<unsigned int, int> m1 { {11, 101}, {12, 202}, {13, 303} };

    {
        // check we copy values not pointers by going out of scope
        MapType<unsigned int, int> m2 { {1, 10} };
        m1 = m2;

        EXPECT_TRUE(m1 == m2);
    }

    EXPECT_TRUE(m1.count(11) == 0);
    EXPECT_TRUE(m1.size() == 1);
    EXPECT_TRUE(m1[1] == 10);

    {
        MapType<unsigned int, int> m2 { {5, 500}, {6, 600} };
        m1 = std::move(m2);
    }

    EXPECT_TRUE(m1.count(1) == 0);
    EXPECT_TRUE(m1.size() == 2);
    EXPECT_TRUE(m1[5] == 500);
    EXPECT_TRUE(m1[6] == 600);
}


// Iterators

template <template<typename...> class MapType>
void test_iterators()
{
    MapType<int, int> m;
    int n = 10;
    for (int i = 0; i < n; i ++)
        m.emplace(i, i * 10);

    // test both non-const and const overloads of .begin()
    EXPECT_TRUE(m.cbegin() == m.begin());
    EXPECT_TRUE(m.cbegin() == ( const_cast<const MapType<int, int>&>(m) ).begin());

    EXPECT_EQ(std::distance(m.begin(),  m.end()) , m.size());
    EXPECT_EQ(std::distance(m.cbegin(), m.cend()), m.size());

    std::multiset<std::pair<int, int>> s;
    for (const auto& p : m)
        s.insert(p);

    EXPECT_EQ(s.size(), n);
    auto it = s.begin();
    for (int i = 0; i < n; i ++)
    {
        EXPECT_EQ(*it, std::make_pair(i, i * 10));
        std::advance(it, 1);
    }
}


// Modifiers

template <template<typename...> class MapType>
void test_clear()
{
    MapType<unsigned int, int> m { {1, 10}, {2, 20}, {3, 30} };
    const size_t bucket_count_before_clear = m.bucket_count();

    m.clear();

    EXPECT_TRUE(m.empty());
    EXPECT_TRUE(m.bucket_count() == bucket_count_before_clear);
}

template <template<typename...> class MapType>
void test_insert()
{
    MapType<std::string, std::string> m { {"10", "10"}, {"20", "20"}, {"30", "30"} };
    using value_type = typename decltype(m)::value_type;
    using iterator   = typename decltype(m)::iterator;

    {
        const std::string key("(1)");
        std::string value("1");
        const value_type p = std::make_pair(key, value);
        // (1) const value_type& value
        std::pair<iterator, bool> ret = m.insert(p);

        EXPECT_TRUE(m.size() == 4);
        EXPECT_TRUE(ret.second == true);
        EXPECT_TRUE(ret.first->first  == key);
        EXPECT_TRUE(ret.first->second == value);
        EXPECT_TRUE(m[key] == value);
    }

    {
        const std::string key("(1)");
        std::string value("2");
        value_type p(key, value);
        // (1) const value_type&
        std::pair<iterator, bool> ret = m.insert(p);
        // (1) value_type&&
        ret = m.insert( {key, value} );
        ret = m.insert(std::move(p));

        EXPECT_TRUE(m.size() == 4);
        EXPECT_TRUE(ret.second == false);      // key already exists
        EXPECT_TRUE(ret.first == m.find(key)); // element which prevented insertion
        EXPECT_TRUE(m[key] != value);
    }

    {
        const std::string key("(2)");
        std::string value("2");
        // (2) P&& value, std::is_constructible<value_type, P&&>::value == true
        std::pair<iterator, bool> ret = m.insert( std::make_pair(key, value) );

        EXPECT_TRUE(m.size() == 5);
        EXPECT_TRUE(ret.second == true);
        EXPECT_TRUE(ret.first->first  == key);
        EXPECT_TRUE(ret.first->second == value);
        EXPECT_TRUE(m[key] == value);
    }

    {
        const std::vector<std::pair<std::string, std::string>> v {
            { "0", "0" }
          , { "1", "1" }
          , { "20", "2" } // key that already exists
          , { "3", "3" }
          , { "4", "4" }
        };

        // (5) InputIt first, InputIt last
        m.insert( v.begin(), v.end() );

        EXPECT_TRUE(m.size() == 9); // "20" was not inserted again
        EXPECT_TRUE(m["0"] == "0");
        EXPECT_TRUE(m.count("2") == false);
        EXPECT_TRUE(m["20"] == "20");
    }

    {
        // (6) std::initializer_list<value_type> ilist
        m.insert( { {"1", "11"}, {"2", "22"}, {"3", "32"}  } );

        EXPECT_TRUE(m.size() == 10);
        EXPECT_TRUE(m["1"] == "1");
        EXPECT_TRUE(m["2"] == "22");
        EXPECT_TRUE(m["3"] == "3");
    }
}

template <template<typename...> class MapType>
void test_emplace()
{
    MapType<std::string, std::string> m { {"10", "10"}, {"20", "20"}, {"30", "30"} };
    using value_type = typename decltype(m)::value_type;
    using iterator   = typename decltype(m)::iterator;

    const std::string key("(1)");
    std::string value("1");
    const value_type p = std::make_pair(key, value);
    // (1) Args&&... args
    std::pair<iterator, bool> ret = m.emplace(key.c_str(), value);

    EXPECT_TRUE(m.size() == 4);
    EXPECT_TRUE(ret.second == true);
    EXPECT_TRUE(ret.first->first  == key);
    EXPECT_TRUE(ret.first->second == value);
    EXPECT_TRUE(m[key] == value);

    // fail for existing keys
    ret = m.emplace(key.c_str(), "new value");
    EXPECT_TRUE(m.size() == 4);
    EXPECT_TRUE(ret.second == false);
    EXPECT_TRUE(ret.first->first  == key);   // element which prevented insertion
    EXPECT_TRUE(ret.first->second == value);
    EXPECT_TRUE(m[key] == value);
}

template <template<typename...> class MapType>
void test_erase_interface()
{
    MapType<std::string, std::string> m { {"10", "10"}, {"20", "20"}, {"30", "30"} };
    using const_iterator = typename decltype(m)::const_iterator;

    // (1) const_iterator
    m.erase(m.cbegin());
    EXPECT_TRUE(m.size() == 2);

    // (2) const_iterator range
    for (int i = 0; i < 5; i ++)
    {
        std::string s = std::to_string(i);
        m.emplace(s, s);
    }

    const_iterator first = std::next(m.cbegin(), 4);
    const_iterator last  = std::next(m.cbegin(), 6);
    m.erase(first, last);
    EXPECT_TRUE(m.size() == 5);

    // (3) const key_type&
    std::string new_key = "key";
    m[new_key] = "exists";
    int num_removed = m.erase(new_key);
    EXPECT_TRUE(m.size() == 5);
    EXPECT_TRUE(m.count(new_key) == 0);
    EXPECT_TRUE(num_removed == 1);

    num_removed = m.erase(new_key);
    EXPECT_TRUE(num_removed == 0);
    EXPECT_TRUE(m.size() == 5);
}

template <template<typename...> class MapType>
void test_erase_sentinels()
{
    // Test sentinel logic after deleting buckets with only 1 element.
    MapType<std::string, std::string> m;
    std::string val("666");
    int max_keys = 200;
    for (int i = 0; i < max_keys; i ++)
        m.emplace(std::to_string(i), val);
    EXPECT_EQ(m.size(), max_keys);

    for (int i = 50 ; i < 100; i ++)
        m.erase(std::to_string(i));
    EXPECT_EQ(m.size(), max_keys - 50);

    int num_keys = 0;
    for (int i = 0; i < max_keys; i ++)
    {
        const std::string key(std::to_string(i));
        if (m.find(key) != m.end())
            ++ num_keys;
    }
    EXPECT_EQ(num_keys, m.size());
}

template <template<typename...> class MapType>
void test_erase()
{
    test_erase_interface<MapType>();
    test_erase_sentinels<MapType>();
}

template <template<typename...> class MapType>
void test_swap()
{
    MapType<std::string, std::string> m1 { {"10", "10"}, {"20", "20"}, {"30", "30"} };
    MapType<std::string, std::string> m2;
    m2.swap(m1);
    EXPECT_TRUE(m1.size() == 0);
    EXPECT_TRUE(m2.size() == 3);
}

template <template<typename...> class MapType>
void test_modifiers()
{
    test_clear<MapType>();
    test_insert<MapType>();
    test_emplace<MapType>();
    test_erase<MapType>();
    test_swap<MapType>();
}


// Lookup
template <template<typename...> class MapType>
void test_lookup()
{
    MapType<std::string, std::string> m;

    // operator[]
    m["10"] = "10"; // insert (and rehash for si::unordered_map)
    EXPECT_EQ(m.size(), 1);
    m["10"] = "20"; // overwrite
    EXPECT_EQ(m.size(), 1);
    EXPECT_TRUE(m.bucket_count() >= 1);
    m["11"] = "22"; // trigger a rehash for si::unordered_map
    EXPECT_EQ(m.size(), 2);
    EXPECT_TRUE(m.bucket_count() >= 2);

    const std::string key("11");
    EXPECT_EQ(m[key], "22");
    EXPECT_EQ(m["11"], "22");;

    // at
    std::string val = m.at("10");
    EXPECT_EQ(val, "20");
    EXPECT_THROW(m.at("100"), std::out_of_range);

    // count
    EXPECT_EQ(m.count("10"), 1);
    EXPECT_EQ(m.count("20"), 0);

    // find
    using iterator = typename decltype(m)::iterator;
    iterator it = m.find("10");
    EXPECT_EQ(it->first, "10");
    it = m.find("20");
    EXPECT_EQ(it, m.end());

    // equal_range
    std::pair<iterator, iterator> ret = m.equal_range("10");
    it = m.find("10");
    EXPECT_EQ(*(ret.first), *it);
    EXPECT_EQ(ret.second, std::next(it));

    ret = m.equal_range("20");
    EXPECT_TRUE(ret.first == m.end());
    EXPECT_TRUE(ret.second == m.end());
}


// Hash policy
template <template<typename...> class MapType>
void test_hash_policy()
{
    MapType<int, int> m;

    EXPECT_TRUE(m.load_factor() == 0);

    m.rehash(10);
    EXPECT_TRUE(m.bucket_count() >= 10);
    // Initially we only had 1 bucket, so inserting 2 elements would
    // segfault if d_buckets had a size smaller than the new bucket count.
    m.emplace(1, 1);
    m.emplace(2, 2);

    m.reserve(100);
    EXPECT_TRUE(m.bucket_count() >= 100);
}


// Observers
template <template<typename...> class MapType>
void test_observers()
{
    MapType<int, int> m;
    EXPECT_TRUE((std::is_same<decltype(m.key_eq()),        typename decltype(m)::key_equal>::value));
    EXPECT_TRUE((std::is_same<decltype(m.hash_function()), typename decltype(m)::hasher   >::value));
}


// Non-member functions
template <template<typename...> class MapType>
void test_non_member_functions()
{
    MapType<int, int> m1{ {1, 1}, {2, 2} };
    MapType<int, int> m2{ {2, 2}, {1, 1} };
    MapType<int, int> m3{ {2, 2} };

    EXPECT_TRUE(m2 == m1);
    EXPECT_TRUE(m2 != m3);
    std::swap(m1, m3);
    EXPECT_TRUE(m2 != m1);
    EXPECT_TRUE(m2 == m3);
}


// Main correctness test
template <template<typename...> class MapType>
void test_map_interface()
{
    test_constructors<MapType>();

    test_assignment_operator<MapType>();
    test_iterators<MapType>();

    test_modifiers<MapType>();
    test_lookup<MapType>();

    test_hash_policy<MapType>();
    test_observers<MapType>();
    test_non_member_functions<MapType>();
}


// Confirm the unit tests are correct.
TEST(std_unordered_map, interface)
{
    test_map_interface<std::unordered_map>();
}

// Validate our implementations using the same tests.
TEST(si_unordered_map, interface)
{
    test_map_interface<si::unordered_map>();
}

TEST(si_flat_hash_map, interface)
{
    test_map_interface<si::flat_hash_map>();
}

