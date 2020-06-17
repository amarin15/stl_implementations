#ifndef SI_TUPLE_H
#define SI_TUPLE_H

#include <type_traits>


namespace si {

// More detailed implementations
// http://mitchnull.blogspot.com/2012/06/c11-tuple-implementation-details-part-1.html
// https://github.com/llvm/llvm-project/blob/master/libcxx/include/tuple
// https://github.com/microsoft/STL/blob/master/stl/inc/tuple


// ~~ tuple ~~

// Primary template.
// Empty tuple will have a default np-op constructor.
template <typename... Tail>
class tuple
{};

template <typename Head, typename... Tail>
class tuple<Head, Tail...> : tuple<Tail...> // inherit values in Tail
{
public:
    tuple (Head head, Tail... tail)
        : BaseType(tail...) // Call the base constructor and instantiate it.
        , d_head(head)
    {}

    template <typename... OtherTail>
    tuple (const tuple<OtherTail...>& other)
        : BaseType(other.tail())
        , d_head(other.head())
    {}

protected:
    // own the first value
    Head d_head;

private:
    // alias the base which contains the Tail elements
    using BaseType = tuple<Tail...>;

    template <typename Ret, size_t N>
    friend class getNth;

    Head& head()
    {
        return d_head;
    }

    const Head& head() const
    {
        return d_head;
    }

    BaseType& tail()
    {
        return *this;
    }

    const BaseType& tail() const
    {
        return *this;
    }
};



// ~~ make_tuple ~~
template <typename... Tail>
tuple<Tail...> make_tuple(Tail&&... tail)
{
    return tuple<Tail...>(tail...);
}



// ~~ get ~~


// Helper to select a type from N types.
// general case; never instantiated
template <size_t N, typename... Tail>
struct select;

template <typename T, typename ... Tail>
struct select<0, T, Tail...>
{
    using type = T;
};

template <size_t N, typename T, typename ... Tail>
struct select <N, T, Tail...>
    : select <N - 1, Tail...>
{};



// Helper to recursively get N-th value from a tuple.
template <typename Ret, size_t N>
struct getNth
{
    template <typename Tuple>
    static Ret& get(Tuple& t)
    {
        return getNth<Ret, N - 1>::get(t.tail());
    }

    template <typename T>
    static const Ret& get(const T& t)
    {
        return getNth<Ret, N -  1>::get(t.tail());
    }
};

template <typename Ret>
struct getNth<Ret, 0>
{
    template<typename Tuple>
    static Ret& get(Tuple& t)
    {
        return t.head();
    }

    template<typename Tuple>
    static const Ret& get(const Tuple& t)
    {
        return t.head();
    }
};


// get that returns a reference
template <size_t N, typename Head, typename... Tail>
typename select<N, Head, Tail...>::type&
get(tuple<Head, Tail...>& t)
{
    using Ret = typename select<N, Head, Tail...>::type;
    return getNth<Ret, N>::get(t);
}

// get that returns a const reference
template <size_t N, typename Head, typename... Tail>
const typename select<N, Head, Tail...>::type&
get(const tuple<Head, Tail...>& t)
{
    using Ret = typename select<N, Head, Tail...>::type;
    return getNth<Ret, N>::get(t);
}


} // namespace si

#endif

