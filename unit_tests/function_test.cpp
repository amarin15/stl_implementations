#include <gtest/gtest.h>

#include <functional>

#include <si_function.h>

int sum(int a, int b)
{
    return a + b;
}

TEST(si_function, from_free_function)
{
    // Given
    si::function<int(int, int)> f(sum);

    // When
    auto actual = f(2, 3);

    // Then
    EXPECT_EQ(actual, 5);
}
