#include <gtest/gtest.h>

#include <si_malloc.h>

struct Node
{
    double d;
    float f;
    int i;
};

TEST(MallocShould, AllocateAndFreeInt)
{
    int* nr = (int*)si::malloc(sizeof(int));
    ASSERT_TRUE(nr);

    *nr = 2;
    EXPECT_EQ(*nr, 2);

    si::free(nr);
}

TEST(MallocShould, AllocateAndFreeDouble)
{
    double* nr = (double*)si::malloc(sizeof(double));
    ASSERT_TRUE(nr);

    *nr = 2.2;
    EXPECT_DOUBLE_EQ(*nr, 2.2);

    si::free(nr);
}

TEST(MallocShould, AllocateAndFreeNode)
{
    Node* node = (Node*)si::malloc(sizeof(Node));
    ASSERT_TRUE(node);

    node->d = 2.2;
    node->f = 2.1;
    node->i = 2;
    EXPECT_DOUBLE_EQ(node->d, 2.2);
    EXPECT_FLOAT_EQ(node->f, 2.1);
    EXPECT_EQ(node->i, 2);

    si::free(node);
}

TEST(MallocShould, AllocateAndFreeMediumMemory)
{
    const int count = 100;

    Node* nodes = (Node*)si::malloc(count * sizeof(Node));
    ASSERT_TRUE(nodes);

    for (int i = 0; i < count; ++i)
    {
        Node* node = nodes + i;

        node->d = 2.2 + i;
        node->f = 2.1 + i;
        node->i = 2 + i;
    }

    for (int i = 0; i < count; ++i)
    {
        Node* node = nodes + i;

        EXPECT_DOUBLE_EQ(node->d, 2.2 + i);
        EXPECT_FLOAT_EQ(node->f, 2.1 + i);
        EXPECT_EQ(node->i, 2 + i);
    }

    si::free(nodes);
}

TEST(MallocShould, DoMultipleAllocationsAndFrees)
{
    Node* node = (Node*)si::malloc(sizeof(Node));
    ASSERT_TRUE(node);
    node->d = 2.2;
    node->f = 2.1;
    node->i = 2;

    int* nr = (int*)si::malloc(sizeof(int));
    ASSERT_TRUE(nr);
    *nr = 33;

    EXPECT_EQ(*nr, 33);
    si::free(nr);

    EXPECT_DOUBLE_EQ(node->d, 2.2);
    EXPECT_FLOAT_EQ(node->f, 2.1);
    EXPECT_EQ(node->i, 2);

    si::free(node);
}
