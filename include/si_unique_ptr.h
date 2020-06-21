#ifndef SI_UNIQUE_PTR_H
#define SI_UNIQUE_PTR_H

#include <memory> // default_delete

namespace si {

template <typename T, typename D = std::default_delete<T>>
class unique_ptr
{
public:
    // CONSTRUCTORS
    unique_ptr()
        : d_data(nullptr)
    {}

    explicit unique_ptr(T* value)
    {
        d_data = value;
    }

    unique_ptr(unique_ptr<T>&& other)
    {
        _acquire(std::forward<unique_ptr<T>>(other));
    }

    ~unique_ptr()
    {
        _delete();
    }

    // MODIFIERS
    unique_ptr<T>& operator=(const unique_ptr<T>& other) = delete;

    unique_ptr<T>& operator=(unique_ptr<T>&& other)
    {
        _acquire(std::forward<unique_ptr<T>>(other));
        return *this;
    }

    void operator=(nullptr_t) noexcept
    {
        _delete();
    }

    T* release() noexcept
    {
        T* ptr = d_data;
        d_data = nullptr; // don't delete

        return ptr;
    }

    void reset(T* value = nullptr) noexcept
    {
        _delete();
        d_data = value;
    }

    // ACCESSORS
    inline T* get() const
    {
        return d_data;
    }

    inline operator bool() const noexcept
    {
        return d_data != nullptr;
    }

    inline T& operator*() const noexcept
    {
        assert(d_data != nullptr);
        return *d_data;
    }

    inline T* operator->() const noexcept
    {
        return get();
    }

    inline bool operator==(const unique_ptr<T>& other) const noexcept
    {
        return this == &other;
    }

    inline bool operator!=(const unique_ptr<T>& other) const noexcept
    {
        return this != &other;
    }

    inline bool operator==(nullptr_t) const noexcept
    {
        return d_data == nullptr;
    }

    inline bool operator!=(nullptr_t) const noexcept
    {
        return d_data != nullptr;
    }

private:
    void _delete() noexcept
    {
        if (d_data != nullptr)
        {
            delete d_data;
            d_data = nullptr;
        }
    }

    void _acquire(unique_ptr<T>&& other) noexcept
    {
        d_data = other.get();
        other.d_data = nullptr;
    }

    T* d_data;
};


} // namespace si

#endif

