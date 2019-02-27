#ifndef SI_TUPLE_H
#define SI_TUPLE_H

#include <utility> // std::forward


namespace si {

// If we want to implement an iterative version of tuple,
// we can use multiple inheritance instead of recursion.
// http://mitchnull.blogspot.com/2012/06/c11-tuple-implementation-details-part-1.html


// Each member of a tuple will have its value stored in a leaf.
// The non-type template parameter `N` allows the `get` function
// to find the value in O(1) time.
template<size_t N, typename T>
struct tuple_leaf
{
    T value;

    tuple_leaf(T&& t)
        : value(std::forward<T>(t))
    {}

    // copy constructor
    tuple_leaf(const tuple_leaf<N, T>& other)
        : value(other.value)
    {}

    // move constructor
    tuple_leaf(tuple_leaf<N, T>&& other)
        : value(std::move(other.value))
    {}
};

// `tuple_impl` is a proxy for the `tuple` class that has an extra
// non-type template parameter `N`.
template <size_t N, typename ... Ts>
struct tuple_impl;

// Base case: empty tuple.
template <size_t N>
struct tuple_impl<N>
{};

// Recursive specialization.
template <size_t N, typename HeadT, typename ... TailTs>
struct tuple_impl<N, HeadT, TailTs...>
    : public tuple_leaf<N, HeadT>         // inherit the N-th leaf value.
    , public tuple_impl<N + 1, TailTs...> // define recursively.
{
    tuple_impl<N, HeadT, TailTs...>(HeadT&& head, TailTs&&... tail)
        : tuple_leaf<N, HeadT>(std::forward<HeadT>(head))
        , tuple_impl<N + 1, TailTs...>(std::forward<TailTs>(tail)...)
    {}

    // copy constructor
    tuple_impl<N, HeadT, TailTs...>(const tuple_impl<N, HeadT, TailTs...>& other)
        : tuple_leaf<N, HeadT>(other)
        , tuple_impl<N + 1, TailTs...>(other)
    {}

    // move constructor
    tuple_impl<N, HeadT, TailTs...>(tuple_impl<N, HeadT, TailTs...>&& other)
        : tuple_leaf<N, HeadT>(std::move(other))
        , tuple_impl<N + 1, TailTs...>(std::move(other))
    {}

    // assignment operator
    tuple_impl<N, HeadT, TailTs...>& operator=(const tuple_impl<N, HeadT, TailTs...>& other)
    {
        this->tuple_leaf<N, HeadT>::value = other.tuple_leaf<N, HeadT>::value;
        return *this;
    }

    tuple_impl<N, HeadT, TailTs...>& operator=(tuple_impl<N, HeadT, TailTs...>&& other)
    {
        this->tuple_leaf<N, HeadT>::value = std::move(other.tuple_leaf<N, HeadT>::value);
        return *this;
    }
};


// Define tuple as a more convenient alias of tuple_impl.
template <typename ... Ts>
using tuple = tuple_impl<0, Ts...>;


// Define our get function to obtain the N-th item in a tuple.
template<size_t N, typename HeadT, typename ... TailTs>
HeadT& get(tuple_impl<N, HeadT, TailTs...>& tpl)
{
    // Fully qualified name for the member, to find the right one.
    return tpl.tuple_leaf<N, HeadT>::value;
}


// Define our make_tuple function.
template <typename ... Ts>
// `args` is a function parameter of a function template declared as
// an rvalue reference to a cv-unqualified type template parameter of
// the same function template, which makes it a forwarding reference.
tuple<Ts...> make_tuple(Ts&&... args)
{
    return tuple<Ts...>(std::forward<Ts>(args)...);
}

} // namespace si

#endif

