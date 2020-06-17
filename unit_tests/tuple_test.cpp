#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include <tuple>
#include <si_tuple.h>


TEST(si_tuple, copy_constructor)
{
    // Given
    si::tuple<int, double> tpl(1, 2.3);

    // When
    si::tuple<int, double> tpl_copy(tpl);

    // Then
    EXPECT_EQ(si::get<0>(tpl_copy), 1);
    EXPECT_EQ(si::get<1>(tpl_copy), 2.3);
}

TEST(si_tuple, move_constructor)
{
    auto tpl = si::make_tuple(1, std::string("2"), 3.45);

    auto tpl2(std::move(tpl));

    EXPECT_EQ(si::get<0>(tpl2), 1);
    EXPECT_EQ(si::get<1>(tpl2), "2");
    EXPECT_EQ(si::get<2>(tpl2), 3.45);
}

// TEST(si_tuple, assignment_operator)
// {
//     si::tuple<int, double> tpl1 = si::make_tuple(1, 2.34);
//     si::tuple<int, double> tpl2 = si::make_tuple(10, 23.4);

//     tpl1 = tpl2;

//     EXPECT_EQ(si::get<0>(tpl1), 10);
//     EXPECT_EQ(si::get<1>(tpl1), 23.4);

//     si::get<0>(tpl1) = 5;
//     EXPECT_EQ(si::get<0>(tpl2), 10);

//     si::get<0>(tpl2) = 6;
//     EXPECT_EQ(si::get<0>(tpl1), 5);

//     tpl1 = std::move(tpl2);
//     EXPECT_EQ(si::get<0>(tpl1), 6);
//     EXPECT_EQ(si::get<1>(tpl1), 23.4);
// }

TEST(si_tuple, make_tuple)
{
    auto tpl = si::make_tuple(1, std::string("2"), 3.14);

    EXPECT_TRUE((std::is_same<decltype(tpl),
                              si::tuple<int, std::string, double>>()));
    EXPECT_EQ(si::get<0>(tpl), 1);
    EXPECT_EQ(si::get<1>(tpl), std::string("2"));
    EXPECT_DOUBLE_EQ(si::get<2>(tpl), 3.14);
}

TEST(si_tuple, get_ref)
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

TEST(si_tuple, get_const_ref)
{
    const std::tuple<int, double, char> tpl(1, 2.3, '4');

    EXPECT_DOUBLE_EQ(std::get<1>(tpl), 2.3);
}

// TEST(si_tuple, tie)
// {
//     si::tuple<int, double, std::string, int> tpl = si::make_tuple(1, 2, "3", 4);

//     int a;
//     double b;
//     std::string c;
//     si::tie(a, b, c, si::ignore) = tpl;

//     EXPECT_EQ(1, a);
//     EXPECT_DOUBLE_EQ(2, b);
//     EXPECT_EQ("3", c);
// }

// TEST(si_tuple, tuple_cat)
// {
//     auto tpl1 = si::make_tuple(1, 2.0);
//     auto tpl2 = si::make_tuple("3", '4');
//     int n = 5;
//     auto concat = si::tuple_cat(tpl1, tpl2, tpl1, si::tie(n));
//     n = 10;

//     int last;
//     double sixth;
//     si::tie(si::ignore, si::ignore, si::ignore, si::ignore, si::ignore, sixth, last) = concat;
//     EXPECT_DOUBLE_EQ(si::get<1>(tpl1), sixth);
//     EXPECT_EQ(n, last); // reference (n was changed after tie inside tuple_cat)
// }

// TEST(si_tuple, tuple_size)
// {
//     auto tpl = si::make_tuple(1, 2, 3);
//     EXPECT_EQ(3, si::tuple_size<decltype(tpl)>::value);
// }


// TEST(si_tuple, equality_operator)
// {
//     si::tuple<int, std::string, double> tpl = si::make_tuple(1, "2", 3.45);
//     si::tuple<int, std::string, double> tpl_copy = tpl;
//     EXPECT_TRUE(tpl_copy == tpl);
// }

// TEST(si_tuple, inequality_operator)
// {
//     si::tuple<int, std::string, double> tpl = si::make_tuple(1, "2", 3.45);
//     si::tuple<int, std::string, double> tpl_copy = tpl;
//     EXPECT_FALSE(tpl_copy != tpl);
// }


// TEST(si_tuple, swap)
// {
//     si::tuple<int, double> tpl = si::make_tuple(1, 3.45);
//     auto tpl_copy = tpl;
//     si::get<0>(tpl_copy) = 2;
//     si::get<1>(tpl_copy) = 6.9;
//     tpl.swap(tpl_copy);
//     EXPECT_EQ(si::get<0>(tpl), 2);
//     EXPECT_EQ(si::get<1>(tpl), 6.9);
//     EXPECT_EQ(si::get<0>(tpl_copy), 1);
//     EXPECT_EQ(si::get<1>(tpl_copy), 3.45);
// }

// TEST(si_tuple, std_swap)
// {
//     si::tuple<int, double> tpl = si::make_tuple(1, 3.45);
//     auto tpl_copy = tpl;
//     si::get<0>(tpl_copy) = 2;
//     si::get<1>(tpl_copy) = 6.9;
//     std::swap(tpl, tpl_copy);
//     EXPECT_EQ(si::get<0>(tpl), 2);
//     EXPECT_EQ(si::get<1>(tpl), 6.9);
//     EXPECT_EQ(si::get<0>(tpl_copy), 1);
//     EXPECT_EQ(si::get<1>(tpl_copy), 3.45);
// }

