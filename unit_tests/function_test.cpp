#include <gtest/gtest.h>

#include <functional>

#include <si_function.h>
#include <string.h>

std::string concat(int a, std::string b)
{
    return std::to_string(a) + b;
}

int sum(int a, int b)
{
    return a + b;
}

TEST(si_function, from_free_function)
{
    // Given
    si::function<std::string(int, std::string)> f(concat);

    // When
    auto actual = f(2, "3");

    // Then
    EXPECT_EQ(actual, "23");
}

TEST(si_function, from_lvalue_si_function)
{
    // Given
    si::function<int(int, int)> f(sum);
    si::function<int(int, int)> f2(f);

    // When
    auto actual = f2(2, 3);

    // Then
    EXPECT_EQ(actual, 5);
}

TEST(si_function, from_rvalue_si_function)
{
    // Given
    si::function<std::string(int, std::string)> f(concat);
    si::function<std::string(int, std::string)> f2(std::move(f));

    // When
    auto actual = f2(2, "3");

    // Then
    EXPECT_EQ(actual, "23");
}

