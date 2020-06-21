#include <gtest/gtest.h>

#include <cassert>

#include <si_ring_buffer.h>

TEST (si_ring_buffer, basic_ops)
{
    CircularBuffer<int> cb(3);
    assert(0 == cb.size());
    try
    {
        cb.pop_front();
        assert(0 == 1);
    }
    catch (const std::exception& err)
    {}

    cb.push_back(10);
    assert(1 == cb.size());
    assert(10 == cb.pop_front());
    assert(0 == cb.size());

    cb.push_back(11);
    cb.push_back(20);
    cb.push_back(30);
    try
    {
        cb.push_back(40);
        assert(0 == 1);
    }
    catch(const std::exception& err)
    {}

    assert(11 == cb.pop_front());
    cb.push_back(100); // test wrap around
    assert(20 == cb.pop_front());
    assert(30 == cb.pop_front());
    assert(100 == cb.pop_front());
    assert(0 == cb.size());
}