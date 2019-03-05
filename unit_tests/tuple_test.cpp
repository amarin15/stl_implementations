#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include <tuple>
#include <si_tuple.h>


// Member functions

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
    auto tpl = si::make_tuple(1, "2", 3.45);

    auto tpl2 = std::move(tpl);

    EXPECT_EQ(si::get<0>(tpl2), 1);
    EXPECT_EQ(si::get<1>(tpl2), "2");
    EXPECT_EQ(si::get<2>(tpl2), 3.45);
}


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

