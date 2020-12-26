#include <gtest/gtest.h>

#include <si_malloc.h>

struct Node
{
    int i;
    double d;
};

TEST(MallocShould, AllocateAndFree)
{
    Node* node = (Node*)si::malloc(sizeof(Node));
    ASSERT_TRUE(node);

    node->i = 2;
    node->d = 2.2;
    EXPECT_EQ(node->i, 2);
    EXPECT_DOUBLE_EQ(node->d, 2.2);

    si::free(node);
}
