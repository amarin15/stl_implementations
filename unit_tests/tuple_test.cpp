#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include <tuple>
#include <si_tuple.h>


TEST(si_tuple, copy_constructor)
{
    // Given
    std::tuple<int, double> tpl(1, 2.3);

    // When
    std::tuple<int, double> tpl_copy(tpl);

    // Then
    EXPECT_EQ(std::get<0>(tpl_copy), 1);
    EXPECT_EQ(std::get<1>(tpl_copy), 2.3);
}

TEST(si_tuple, move_constructor)
{
    auto tpl = std::make_tuple(1, "2", 3.45);

    auto tpl2 = std::move(tpl);

    EXPECT_EQ(std::get<0>(tpl2), 1);
    EXPECT_EQ(std::get<1>(tpl2), "2");
    EXPECT_EQ(std::get<2>(tpl2), 3.45);
}

TEST(si_tuple, assignment_operator)
{
    std::tuple<int, double> tpl1 = std::make_tuple(1, 2.34);
    std::tuple<int, double> tpl2 = std::make_tuple(10, 23.4);

    tpl1 = tpl2;

    EXPECT_EQ(std::get<0>(tpl1), 10);
    EXPECT_EQ(std::get<1>(tpl1), 23.4);

    std::get<0>(tpl1) = 5;
    EXPECT_EQ(std::get<0>(tpl2), 10);

    std::get<0>(tpl2) = 6;
    EXPECT_EQ(std::get<0>(tpl1), 5);

    tpl1 = std::move(tpl2);
    EXPECT_EQ(std::get<0>(tpl1), 6);
    EXPECT_EQ(std::get<1>(tpl1), 23.4);
}


TEST(si_tuple, make_tuple)
{
    {
        auto tpl = std::make_tuple(1, std::string("2"));

        EXPECT_TRUE((std::is_same<decltype(tpl),
                                  std::tuple<int, std::string>>::value));
        EXPECT_EQ(std::get<0>(tpl), 1);
        EXPECT_EQ(std::get<1>(tpl), std::string("2"));
    }

    {
        const std::string& str = "2";
        auto tpl = std::make_tuple(1.0, str);

        EXPECT_TRUE((std::is_same<decltype(tpl),
                                  std::tuple<double, std::string>>::value));
        EXPECT_EQ(std::get<1>(tpl), str);
    }
}

TEST(si_tuple, get)
{
    std::tuple<int, double> tpl = std::make_tuple(1, 2.34);

    auto &a = std::get<0>(tpl);
    auto &b = std::get<1>(tpl);

    EXPECT_TRUE((std::is_same<decltype(a), int&   >::value));
    EXPECT_TRUE((std::is_same<decltype(b), double&>::value));

    a = 10;
    b = 34.5;

    EXPECT_EQ(std::get<0>(tpl), a);
    EXPECT_EQ(std::get<1>(tpl), b);
}

TEST(si_tuple, tie)
{
    std::tuple<int, double, std::string, int> tpl = std::make_tuple(1, 2, "3", 4);

    int a;
    double b;
    std::string c;
    std::tie(a, b, c, std::ignore) = tpl;

    EXPECT_EQ(1, a);
    EXPECT_DOUBLE_EQ(2, b);
    EXPECT_EQ("3", c);
}

TEST(si_tuple, tuple_cat)
{
    auto tpl1 = std::make_tuple(1, 2.0);
    auto tpl2 = std::make_tuple("3", '4');
    int n = 5;
    auto concat = std::tuple_cat(tpl1, tpl2, tpl1, std::tie(n));
    n = 10;

    int last;
    double sixth;
    std::tie(std::ignore, std::ignore, std::ignore, std::ignore, std::ignore, sixth, last) = concat;
    EXPECT_DOUBLE_EQ(std::get<1>(tpl1), sixth);
    EXPECT_EQ(n, last); // reference (n was changed after tie inside tuple_cat)
}

TEST(si_tuple, tuple_size)
{
    auto tpl = std::make_tuple(1, 2, 3);
    EXPECT_EQ(3, std::tuple_size<decltype(tpl)>::value);
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


TEST(si_tuple, swap)
{
    std::tuple<int, double> tpl = std::make_tuple(1, 3.45);
    auto tpl_copy = tpl;
    std::get<0>(tpl_copy) = 2;
    std::get<1>(tpl_copy) = 6.9;
    tpl.swap(tpl_copy);
    EXPECT_EQ(std::get<0>(tpl), 2);
    EXPECT_EQ(std::get<1>(tpl), 6.9);
    EXPECT_EQ(std::get<0>(tpl_copy), 1);
    EXPECT_EQ(std::get<1>(tpl_copy), 3.45);
}

TEST(si_tuple, std_swap)
{
    std::tuple<int, double> tpl = std::make_tuple(1, 3.45);
    auto tpl_copy = tpl;
    std::get<0>(tpl_copy) = 2;
    std::get<1>(tpl_copy) = 6.9;
    std::swap(tpl, tpl_copy);
    EXPECT_EQ(std::get<0>(tpl), 2);
    EXPECT_EQ(std::get<1>(tpl), 6.9);
    EXPECT_EQ(std::get<0>(tpl_copy), 1);
    EXPECT_EQ(std::get<1>(tpl_copy), 3.45);
}
