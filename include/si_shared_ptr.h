#ifndef SI_TUPLE_H
#define SI_TUPLE_H


namespace si {

// Reference counter for shared pointers.
struct shared_count
{
    unsigned long use_count;

    explicit shared_count(unsigned long count = 0)
        : use_count(count)
    {}
};

template <typename T>
class shared_ptr
{
public:
    // CONSTRUCTORS
    shared_ptr()
        : d_data(nullptr)
        , d_counter(nullptr)
    {}

    explicit shared_ptr(T* value)
    {
        _acquire(value);
    }

    shared_ptr(const shared_ptr& other)
    {
        _acquire(other);
    }

    shared_ptr(shared_ptr&& other)
        : d_data(std::move(other.d_data))
        , d_counter(std::move(other.d_counter))
    {
        // Avoid releasing the resources inside other's destructor.
        other.d_counter = nullptr;
        other.d_data = nullptr;
    }

    ~shared_ptr()
    {
        _release();
    }


    // MODIFIERS
    shared_ptr<T>& operator=(T* value)
    {
        reset(value);
        return *this;
    }

    shared_ptr<T>& operator=(const shared_ptr& other)
    {
        if (this != &other)
        {
            _release();
            _acquire(other);
        }
        return *this;
    }

    shared_ptr<T>& operator=(shared_ptr&& other)
    {
        if  (this != &other)
        {
            _release();
            d_data = std::move(other.d_data);
            d_counter = std::move(other.d_counter);

            // Avoid releasing the resources inside other's destructor.
            other.d_counter = nullptr;
            other.d_data = nullptr;
        }

        return *this;
    }

    void reset(T* value = nullptr)
    {
        _release();
        _acquire(value);
    }


    // ACCESSORS
    T& operator*() const noexcept
    {
        assert(d_data != nullptr);
        return *d_data;
    }

    T* operator->() const noexcept
    {
        assert(d_data != nullptr);
        return d_data;
    }

    operator bool() const noexcept
    {
        return d_data != nullptr;
    }

    unsigned long use_count() const noexcept
    {
        if (d_counter != nullptr)
            return d_counter->use_count;
        return 0;
    }

private:
    // RAII implementations
    void _release()
    {
        if (d_counter != nullptr)
        {
            if (d_counter->use_count == 1)
            {
                delete d_counter;
                delete d_data;
            }
            else
            {
                -- d_counter->use_count;
            }

            d_counter = nullptr;
            d_data    = nullptr;
        }
    }

    void _acquire(T* value)
    {
        d_data = value;
        if (value == nullptr)
            // d_counter was either not initialized or released before.
            d_counter = nullptr;
        else
            d_counter = new shared_count(1);
    }

    shared_ptr<T>& _acquire(const shared_ptr<T>& other)
    {
        d_data    = other.d_data;
        d_counter = other.d_counter;

        if (d_counter != nullptr)
            d_counter->use_count ++;

        return *this;
    }

    // DATA
    T*            d_data;
    shared_count* d_counter;
};


} // namespace si

#endif

