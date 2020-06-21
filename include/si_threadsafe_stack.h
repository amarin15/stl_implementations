#ifndef SI_THREADSAFE_STACK_H
#define SI_THREADSAFE_STACK_H

#include <exception>
#include <memory>
#include <mutex>
#include <stack>

namespace si {


template <class T>
class threadsafe_stack
{
public:
    threadsafe_stack()
    {}

    threadsafe_stack(const threadsafe_stack<T>& other)
    {
        std::lock_guard<std::mutex> guard(other.d_mutex);
        d_stack = other.d_stack;
    }

    threadsafe_stack& operator= (const threadsafe_stack<T>& other) = delete;

    void push(const T& val)
    {
        std::lock_guard<std::mutex> guard(d_mutex);
        d_stack.push(val);
    }

    // Combine top and pop to avoid race conditions.
    // Return by value might throw after we already popped from the stack,
    // so we return by pointer or reference instead.
    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> guard(d_mutex);
        if (empty())
            throw std::runtime_error("Empty stack.");

        const auto sptr = std::make_shared<T>(d_stack.top());
        d_stack.pop();
        return sptr;
    }

    void pop(T& result)
    {
        std::lock_guard<std::mutex> guard(d_mutex);
        if (empty())
            throw std::runtime_error("Empty stack.");

        result = d_stack.top();
        d_stack.pop();
    }

    bool empty() const noexcept
    {
        return d_stack.empty();
    }

private:
    std::stack<T>      d_stack;
    mutable std::mutex d_mutex;
};


} // namespace si

#endif

