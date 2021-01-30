#ifndef SI_FUNCTION_H
#define SI_FUNCTION_H

#include <memory>

namespace si {


template<typename Ret, typename ... Args>
struct callable_base
{
    callable_base() = default;
    virtual ~callable_base() {}

    virtual Ret invoke(Args...) = 0;
    virtual std::unique_ptr<callable_base<Ret, Args...>> clone() const = 0;
};

template<typename F, typename Ret, typename ... Args>
struct callable : callable_base<Ret, Args...>
{
    F d_f;

    callable(F const& f) : d_f(f) {}
    callable(F&& f) : d_f(std::move(f)) {}

    Ret invoke(Args... args) override
    {
        return d_f(args...);
    }

    std::unique_ptr<callable_base<Ret, Args...>> clone() const override
    {
        return std::make_unique<callable<F, Ret, Args...>>(d_f);
    }
};

template<typename Ret, typename ... Args>
class function;

template <typename Ret, typename ... Args>
class function<Ret(Args...)>
{
    std::unique_ptr<callable_base<Ret, Args...>> d_callable;

public:
    template<typename F>
    function(F&& f)
        : d_callable(new callable<std::decay_t<F>, Ret, Args...>(std::forward<F>(f)))
    {}

    function(const function& other)
        : d_callable(other.d_callable ? other.d_callable->clone() : nullptr)
    {}

    function(function&& other)
    {
        d_callable = other.d_callable ? std::move(other.d_callable) : nullptr;
    }

    Ret operator()(Args... args)
    {
        return d_callable->invoke(args...);
    }
};


} // namespace si

#endif
