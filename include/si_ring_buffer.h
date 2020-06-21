#ifndef SI_RING_BUFFER_H
#define SI_RING_BUFFER_H

#include <stdexcept>

namespace si {

// TODO multi threaded using atomics
template<typename T>
class ring_buffer
{
public:
    ring_buffer(size_t capacity)
        : d_head(0)
        , d_size(0)
        , d_capacity(capacity)
    {
        d_data = (T*)malloc(capacity * sizeof(T));
    }

    ~ring_buffer()
    {
        if (d_data)
            free(d_data);
    }

    void push_back(const T& val)
    {
        if (d_size == d_capacity)
            throw std::runtime_error("Buffer is full");

        size_t tail = (d_head + d_size) % d_capacity;

        *(d_data + tail) = val;
        d_size ++;
    }

    T pop_front()
    {
        if (d_size == 0)
            throw std::runtime_error("Can't pop, buffer is empty");

        T* prev = d_data + d_head;
        // wrap around
        d_head++;
        if (d_head == d_capacity)
            d_head = 0;

        -- d_size;

        return *prev; 
    }

    size_t size() const
    {
        return d_size;
    }

private:
    size_t d_head; // first element in buffer
    size_t d_size;
    size_t d_capacity;
    T*     d_data;
};

} // namespace si

#endif
