#include <gtest/gtest.h>

#include <si_priority_queue.h>

TEST(PriorityQueueShould, WorkAsMaxHeap)
{
    si::priority_queue<int> pq;

    EXPECT_TRUE(pq.empty());

    pq.push(10);
    EXPECT_FALSE(pq.empty());
    EXPECT_EQ(pq.top(), 10);
    pq.pop();
    EXPECT_EQ(pq.size(), 0);

    pq.push(1);
    EXPECT_EQ(pq.top(), 1);
    pq.push(3);
    EXPECT_EQ(pq.top(), 3);
    pq.push(5);
    EXPECT_EQ(pq.top(), 5);
    pq.push(2);
    EXPECT_EQ(pq.top(), 5);
    pq.push(3);
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
    pq.pop();
    EXPECT_EQ(pq.top(), 2);
    EXPECT_EQ(pq.size(), 2);
}

TEST(PriorityQueueShould, WorkAsMinHeap)
{
    si::priority_queue<int, std::greater<int>> pq;

    EXPECT_TRUE(pq.empty());

    pq.push(10);
    EXPECT_FALSE(pq.empty());
    EXPECT_EQ(pq.top(), 10);
    pq.pop();
    EXPECT_EQ(pq.size(), 0);

    pq.push(3);
    EXPECT_EQ(pq.top(), 3);
    pq.push(2);
    EXPECT_EQ(pq.top(), 2);
    pq.push(5);
    EXPECT_EQ(pq.top(), 2);
    pq.push(1);
    EXPECT_EQ(pq.top(), 1);
    pq.push(3);
    EXPECT_EQ(pq.top(), 1);
    pq.pop();
    EXPECT_EQ(pq.top(), 2);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
    pq.pop();
    EXPECT_EQ(pq.top(), 3);
    pq.pop();
    EXPECT_EQ(pq.top(), 5);
    pq.pop();
    EXPECT_TRUE(pq.empty());
}