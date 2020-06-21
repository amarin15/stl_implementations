#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <si_unique_ptr.h>


// Use a template for the unique_ptr type so that we run the same
// tests on both std::unique_ptr and our implementation.
template <template <class T, class D = std::default_delete<T>> class UniquePtrType>
void test_constructors()
{
    // Default constructor
    {
        UniquePtrType<int> empty_up;
        EXPECT_FALSE(empty_up);
    }

    // Constructor from T*
    {
        UniquePtrType<std::string> up(new std::string("0"));
        EXPECT_TRUE(up);
        EXPECT_EQ(*up, std::string("0"));
    }

    // Move constructor
    {
        UniquePtrType<std::string> up_from(new std::string("val"));
        UniquePtrType<std::string> up_to(std::move(up_from));
        EXPECT_FALSE(up_from);
        EXPECT_TRUE(up_to);
        EXPECT_EQ(*up_to, std::string("val"));
    }
}

template <template <class T, class D = std::default_delete<T>> class UniquePtrType>
void test_assignment_operators()
{
    UniquePtrType<std::string> up;
    EXPECT_FALSE(up);

    UniquePtrType<std::string> up2(new std::string("val"));
    EXPECT_TRUE(up2);
    up = std::move(up2);
    EXPECT_TRUE(up);
    EXPECT_EQ(*up, std::string("val"));
    EXPECT_FALSE(up2);

    up = nullptr;
    EXPECT_FALSE(up);
}

template <template <class T, class D = std::default_delete<T>> class UniquePtrType>
void test_reset()
{
    UniquePtrType<std::string> up;
    up.reset(new std::string("val"));
    EXPECT_TRUE(up);
    EXPECT_EQ(*up, std::string("val"));
    up.reset();
    EXPECT_FALSE(up);
}

template <template <class T, class D = std::default_delete<T>> class UniquePtrType>
void test_release()
{
    UniquePtrType<std::string> up(new std::string("val"));
    EXPECT_TRUE(up);
    std::string* ptr = up.release(); // caller is responsible for deleting
    EXPECT_FALSE(up);
    EXPECT_EQ(*ptr, std::string("val"));
    delete ptr;
}

template <template <class T, class D = std::default_delete<T>> class UniquePtrType>
void test_accessors()
{
    UniquePtrType<std::string> up(new std::string("val"));

    // get
    std::string * ptr = up.get();
    EXPECT_TRUE(ptr);
    EXPECT_EQ(*ptr, std::string("val"));

    // operator bool
    EXPECT_TRUE(up);

    // operator*
    EXPECT_EQ(*up, std::string("val"));

    // operator->
    EXPECT_EQ(up->size(), 3);

    // operator==
    UniquePtrType<std::string> up2(new std::string("val"));
    EXPECT_TRUE(up == up);
    EXPECT_FALSE(up == up2);
    // operator!=
    EXPECT_TRUE(up != up2);
}

template <template <class T, class D = std::default_delete<T>> class UniquePtrType>
void test_unique_ptr_interface()
{
    test_constructors<UniquePtrType>();
    test_assignment_operators<UniquePtrType>();
    test_reset<UniquePtrType>();
    test_release<UniquePtrType>();
    test_accessors<UniquePtrType>();
}


// Confirm the unit tests are correct.
TEST(std_unique_ptr, interface)
{
    test_unique_ptr_interface<std::unique_ptr>();
}

// Validate our implementation using the same tests.
TEST(si_unique_ptr, interface)
{
    test_unique_ptr_interface<si::unique_ptr>();
}
