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
// - comparison operators
//
// Helper classes
// - tuple_size
// - tuple_element
// - ignore


// Member functions

TEST(si_tuple, copy_constructor)
{
    // Given
    si::tuple<int, double> tpl(1, 2.3);

    // When
    auto tpl_copy = tpl;

    // Then
    EXPECT_EQ(si::get<0>(tpl_copy), 1);
    EXPECT_EQ(si::get<1>(tpl_copy), 2.3);
}

TEST(si_tuple, move_constructor)
{
    auto tpl = si::make_tuple(1, "2", 3.45);

    auto tpl2 = std::move(tpl);

    EXPECT_EQ(si::get<0>(tpl2), 1);
    EXPECT_EQ(si::get<1>(tpl2), "2");
    EXPECT_EQ(si::get<2>(tpl2), 3.45);
}

TEST(si_tuple, assignment_operator)
{
    si::tuple<int, double> tpl1 = si::make_tuple(1, 2.34);
    si::tuple<int, double> tpl2 = si::make_tuple(10, 23.4);

    tpl1 = tpl2;

    EXPECT_EQ(si::get<0>(tpl1), 10);
    EXPECT_EQ(si::get<1>(tpl1), 23.4);

    si::get<0>(tpl1) = 5;
    EXPECT_EQ(si::get<0>(tpl2), 10);

    si::get<0>(tpl2) = 6;
    EXPECT_EQ(si::get<0>(tpl1), 5);

    tpl1 = std::move(tpl2);
    EXPECT_EQ(si::get<0>(tpl1), 6);
    EXPECT_EQ(si::get<1>(tpl1), 2.34);
}

/*
TEST(si_tuple, swap)
{
    si::tuple<int, double> tpl = si::make_tuple(1, 3.45);
    auto tpl_copy = tpl;
    si::get<0>(tpl_copy) = 2;
    si::get<1>(tpl_copy) = 6.9;

    tpl.swap(tpl_copy);

    EXPECT_EQ(si::get<0>(tpl), 2);
    EXPECT_EQ(si::get<1>(tpl), 6.9);
    EXPECT_EQ(si::get<0>(tpl_copy), 1);
    EXPECT_EQ(si::get<1>(tpl_copy), 3.45);
}
*/


// Non-member functions

TEST(si_tuple, make_tuple)
{
    {
        auto tpl = si::make_tuple(1, std::string("2"));

        EXPECT_TRUE((std::is_same<decltype(tpl),
                                  si::tuple<int, std::string>>::value));
        EXPECT_EQ(si::get<0>(tpl), 1);
        EXPECT_EQ(si::get<1>(tpl), std::string("2"));
    }

    {
        std::string str = "2";
        auto tpl = si::make_tuple(1.0, str);

        EXPECT_TRUE((std::is_same<decltype(tpl),
                                  si::tuple<double, std::string&>>::value));
        EXPECT_EQ(si::get<1>(tpl), str);
    }
}

TEST(si_tuple, get)
{
    si::tuple<int, double> tpl = si::make_tuple(1, 2.34);

    auto &a = si::get<0>(tpl);
    auto &b = si::get<1>(tpl);

    EXPECT_TRUE((std::is_same<decltype(a), int&   >::value));
    EXPECT_TRUE((std::is_same<decltype(b), double&>::value));

    a = 10;
    b = 34.5;

    EXPECT_EQ(si::get<0>(tpl), a);
    EXPECT_EQ(si::get<1>(tpl), b);
}

/*
TEST(si_tuple, equality_operator)
{
    si::tuple<int, std::string, double> tpl = si::make_tuple(1, "2", 3.45);
    si::tuple<int, std::string, double> tpl_copy = tpl;

    EXPECT_TRUE(tpl_copy == tpl);
}

TEST(si_tuple, inequality_operator)
{
    si::tuple<int, std::string, double> tpl = si::make_tuple(1, "2", 3.45);
    si::tuple<int, std::string, double> tpl_copy = tpl;

    EXPECT_FALSE(tpl_copy != tpl);
}

TEST(si_tuple, std_swap)
{
    si::tuple<int, double> tpl = si::make_tuple(1, 3.45);
    auto tpl_copy = tpl;
    si::get<0>(tpl_copy) = 2;
    si::get<1>(tpl_copy) = 6.9;

    std::swap(tpl, tpl_copy);

    EXPECT_EQ(si::get<0>(tpl), 2);
    EXPECT_EQ(si::get<1>(tpl), 6.9);
    EXPECT_EQ(si::get<0>(tpl_copy), 1);
    EXPECT_EQ(si::get<1>(tpl_copy), 3.45);
}
*/
