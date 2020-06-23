#ifndef SI_LOCKFREE_STACK_H
#define SI_LOCKFREE_STACK_H

#include <atomic>
#include <memory>

namespace si {

// Lock-free stack that leaks nodes. For a version that frees memory, see
// Concurrency In Action, Second Edition by Anthony Williams, Chapter 7.2.2.
// https://github.com/anthonywilliams/ccia_code_samples/blob/main/listings/listing_7.13.cpp
template <class T>
class lockfree_stack
{
public:
    void push(const T& val)
    {
        node* new_node = new node(val, d_head.load());
        while (!d_head.compare_exchange_weak(new_node->next, new_node));
    }

    std::shared_ptr<T> pop()
    {
        node* old_head = d_head.load();
        while (old_head && !d_head.compare_exchange_weak(old_head, old_head->next));
        return old_head ? old_head->data : std::make_shared<T>();
    }

private:
    struct node
    {
        std::shared_ptr<T> data;
        node* next;

        node(const T& val, node* ptr)
            : data(std::make_shared<T>(val))
            , next(ptr)
        {}
    };

    std::atomic<node*> d_head;
};

} // namespace si

#endif
