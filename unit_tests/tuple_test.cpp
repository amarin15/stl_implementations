#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include <tuple>
#include <si_tuple.h>


// TODO:
//
// Member functions
// - all constructors
//
// Non-member functions
// - tie
// - forward_as_tuple
// - tuple_cat
// - remaining comparison operators
//
// Helper classes
// - tuple_size
// - tuple_element
// - ignore


// Member functions

TEST(si_tuple, assigment_operator)
{
    // Given
    std::tuple<int, std::string, double> tpl = std::make_tuple(1, "2", 3.45);

    // When
    auto tpl_copy = tpl;

    // Then
    EXPECT_EQ(tpl_copy, tpl);
}

TEST(si_tuple, swap)
{
    std::tuple<int, std::string, double> tpl = std::make_tuple(1, "2", 3.45);
    auto tpl_copy = tpl;
    std::get<0>(tpl_copy) = 10;

    tpl.swap(tpl_copy);

    EXPECT_EQ(std::get<0>(tpl), 10);
    EXPECT_EQ(std::get<0>(tpl_copy), 1);
}


// Non-member functions

TEST(si_tuple, make_tuple)
{
    auto tpl_char_ptr = std::make_tuple(1, "2", std::string("3.45"));
    std::tuple<int, std::string, std::string> tpl_string = std::make_tuple(1, "2", "3.45");

    EXPECT_TRUE((std::is_same<decltype(tpl_char_ptr), std::tuple<int, const char *, std::string>>::value));
    EXPECT_TRUE((std::is_same<decltype(tpl_string),   std::tuple<int, std::string,  std::string>>::value));
}

TEST(si_tuple, get)
{
    std::tuple<int, std::string, double> tpl = std::make_tuple(1, "2", 3.45);

    auto &a = std::get<0>(tpl);
    auto &b = std::get<1>(tpl);
    auto &c = std::get<2>(tpl);

    EXPECT_TRUE((std::is_same<decltype(a), int&        >::value));
    EXPECT_TRUE((std::is_same<decltype(b), std::string&>::value));
    EXPECT_TRUE((std::is_same<decltype(c), double&     >::value));

    a = 10;
    b = "20";
    c = 34.5;
    EXPECT_EQ(std::get<0>(tpl), a);
    EXPECT_EQ(std::get<1>(tpl), b);
    EXPECT_EQ(std::get<2>(tpl), c);
}

TEST(si_tuple, equality_operator)
{
    std::tuple<int, std::string, double> tpl = std::make_tuple(1, "2", 3.45);
    std::tuple<int, std::string, double> tpl_copy = tpl;
 
    EXPECT_TRUE(tpl_copy == tpl);
}

TEST(si_tuple, inequality_operator)
{
    std::tuple<int, std::string, double> tpl = std::make_tuple(1, "2", 3.45);
    std::tuple<int, std::string, double> tpl_copy = tpl;
 
    EXPECT_FALSE(tpl_copy != tpl);
}

TEST(si_tuple, std_swap)
{
    std::tuple<int, std::string, double> tpl = std::make_tuple(1, "2", 3.45);
    auto tpl_copy = tpl;
    std::get<0>(tpl_copy) = 10;

    std::swap(tpl, tpl_copy);

    EXPECT_EQ(std::get<0>(tpl), 10);
    EXPECT_EQ(std::get<0>(tpl_copy), 1);
}

