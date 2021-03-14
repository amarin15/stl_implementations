#ifndef SI_PRIORITY_QUEUE_H
#define SI_PRIORITY_QUEUE_H

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <stdexcept>

namespace si {

template <typename T, typename Cmp = std::less<T>>
class priority_queue
{
public:
    priority_queue(int capacity = 4)
        : d_size(0)
        , d_capacity(capacity)
    {
        d_heap = (T*)malloc(d_capacity * sizeof(T));
    }

    ~priority_queue()
    {
        free(d_heap);
    }

    void push(T nr)
    {
        expand_if_needed();
        d_heap[d_size++] = nr;

        // redo the heap structure
        unsigned int i = d_size - 1;
        while (i)
        {
            const unsigned int parent = (i - 1) >> 1;
            if (!Cmp()(d_heap[parent], d_heap[i]))
                break;

            std::swap(d_heap[parent], d_heap[i]);
            i = parent;
        }
    }

    void pop()
    {
        if (empty())
            throw std::runtime_error("Queue is empty.");

        // "pop" by just moving to the back
        std::swap(d_heap[--d_size], d_heap[0]);

        // redo the heap structure
        unsigned int i = 0;
        while (true)
        {
            const unsigned int left  = i * 2 + 1;
            const unsigned int right = i * 2 + 2;

            if (left >= d_size)
                break;
            unsigned int child = left;
            if (right < d_size && Cmp()(d_heap[left], d_heap[right]))
                child = right;

            if (!Cmp()(d_heap[i], d_heap[child]))
                break;

            std::swap(d_heap[i], d_heap[child]);
            i = child;
        }

        shrink_if_needed();
    }

    inline T top() const
    {
        if (empty())
            throw std::runtime_error("Queue is empty.");
        return d_heap[0];
    }

    inline bool empty() const
    {
        return d_size == 0;
    }

    inline unsigned int size() const
    {
        return d_size;
    }

private:
    // doubles the capacity if full
    void expand_if_needed()
    {
        if (d_size == d_capacity)
        {
            d_capacity <<= 1;
            d_heap = (T*)realloc(d_heap, d_capacity * sizeof(T));
        }
    }

    // halves the capacity if we reach 25%
    void shrink_if_needed()
    {
        if (d_size * 4 < d_capacity)
        {
            d_capacity >>= 1;
            d_heap = (T*)realloc(d_heap, d_capacity * sizeof(T));
        }
    }

    unsigned long long d_size;
    unsigned long long d_capacity;
    T*                 d_heap;
};

} // namespace si

#endif
