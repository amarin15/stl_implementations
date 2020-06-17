#ifndef SI_TUPLE_H
#define SI_TUPLE_H

#include <type_traits>
#include <utility>


namespace si {

// More detailed implementations
// http://mitchnull.blogspot.com/2012/06/c11-tuple-implementation-details-part-1.html
// https://github.com/llvm/llvm-project/blob/master/libcxx/include/tuple
// https://github.com/microsoft/STL/blob/master/stl/inc/tuple


// ~~ tuple ~~

// Primary template.
// Empty tuple will have a default np-op constructor.
template <class... Tail>
class tuple
{};

template <class Head, class... Tail>
class tuple<Head, Tail...> : public tuple<Tail...> // inherit values in Tail
{
public:
    // Constructors
    tuple (const Head& head, const Tail&... tail)
        : BaseType(tail...) // Call the base constructor and instantiate it.
        , d_head(head)
    {}

    template <class... Ts>
    tuple (const tuple<Ts...>& other)
        : BaseType(other.tail())
        , d_head(other.head())
    {}

protected:
    // own the first value
    Head d_head;

private:
    // alias the base which contains the Tail elements
    using BaseType = tuple<Tail...>;

    template <class Ret, size_t N>
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

template <class... Tail>
tuple<Tail...> make_tuple(Tail&&... tail)
{
    return tuple<Tail...>(std::forward<Tail>(tail)...);
}


// ~~ tuple_element ~~

// Select the N-th type from a tuple.
// general case; never instantiated
template <size_t N, class>
struct tuple_element;

template <size_t N, class Head, class... Tail>
struct tuple_element <N, tuple<Head, Tail...>>
    : tuple_element <N - 1, tuple<Tail...>>
{};

template <class Head, class ... Tail>
struct tuple_element<0, tuple<Head, Tail...>>
{
    using type = Head;
};


// ~~ get ~~

// Helper to get N-th value from a tuple.
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
typename tuple_element<N, tuple<Head, Tail...>>::type&  // Ret&
get(tuple<Head, Tail...>& t)
{
    using Ret = typename tuple_element<N, tuple<Head, Tail...>>::type;
    return getNth<Ret, N>::get(t);
}

// get that returns a const reference
template <size_t N, typename Head, typename... Tail>
const typename tuple_element<N, tuple<Head, Tail...>>::type& // const Ret&
get(const tuple<Head, Tail...>& t)
{
    using Ret = typename tuple_element<N, tuple<Head, Tail...>>::type;
    return getNth<Ret, N>::get(t);
}


} // namespace si

#endif
