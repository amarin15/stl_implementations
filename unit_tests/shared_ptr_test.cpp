#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <si_shared_ptr.h>


// Use a template for the shared_ptr type so that we run the same
// tests on both std::shared_ptr and our implementation.
template <template<typename> class SharedPtrType>
void test_constructors()
{
    // Default constructor
    {
        SharedPtrType<int> empty_sp;
        EXPECT_FALSE(empty_sp);
        EXPECT_EQ(empty_sp.use_count(), 0);
    }

    // Constructor from T*
    {
        SharedPtrType<int> empty_sp(nullptr);
        EXPECT_FALSE(empty_sp);
        EXPECT_EQ(empty_sp.use_count(), 0);

        SharedPtrType<std::string> sp(new std::string("0"));
        EXPECT_TRUE(sp);
        EXPECT_EQ(sp.use_count(), 1);
    }

    // Copy constructor
    {
        SharedPtrType<int> empty_sp;
        SharedPtrType<int> sp_empty_copy(empty_sp);
        EXPECT_FALSE(sp_empty_copy);
        EXPECT_EQ(sp_empty_copy.use_count(), 0);

        SharedPtrType<int> sp(new int(0));
        SharedPtrType<int> sp_copy(sp);
        EXPECT_TRUE(sp_copy);
        EXPECT_TRUE(sp);
        EXPECT_EQ(*sp, *sp_copy);
        EXPECT_EQ(sp.use_count(), 2);
        EXPECT_EQ(sp_copy.use_count(), 2);
    }

    // Move constructor
    {
        SharedPtrType<int> empty_sp;
        SharedPtrType<int> sp_empty_move(std::move(empty_sp));
        EXPECT_FALSE(sp_empty_move);
        EXPECT_EQ(sp_empty_move.use_count(), 0);

        SharedPtrType<int> sp(new int(1));
        EXPECT_TRUE(sp);
        SharedPtrType<int> sp_move(std::move(sp));
        EXPECT_TRUE(sp_move);
        EXPECT_FALSE(sp);
        EXPECT_EQ(sp.use_count(), 0);
        EXPECT_EQ(sp_move.use_count(), 1);
    }

    // Destructor
    {
        SharedPtrType<int> sp(new int(1));
        EXPECT_EQ(sp.use_count(), 1);
        {
            SharedPtrType<int> sp_copy(sp);
            EXPECT_EQ(sp.use_count(), 2);
        }
        EXPECT_EQ(sp.use_count(), 1);
    }
}

template <template<typename> class SharedPtrType>
void test_assignment_operators()
{
    // Copy assignment operator
    {
        // initially empty
        SharedPtrType<int> sp;
        SharedPtrType<int> sp_from(new int(11));
        sp = sp_from;
        EXPECT_TRUE(sp);
        EXPECT_EQ(*sp, 11);
        EXPECT_EQ(sp.use_count(), 2);
        EXPECT_EQ(sp_from.use_count(), 2);

        // release the previous pointer before assignment
        SharedPtrType<int> sp_release(sp);
        EXPECT_EQ(*sp_release, 11);
        EXPECT_EQ(sp_release.use_count(), 3);
        EXPECT_EQ(sp.use_count(), 3);
        SharedPtrType<int> empty_sp;
        sp_release = empty_sp;
        EXPECT_FALSE(sp_release);
        EXPECT_EQ(sp_release.use_count(), 0);
        EXPECT_EQ(sp.use_count(), 2);
        EXPECT_EQ(empty_sp.use_count(), 0);

        // test that we don't acquire *this
        SharedPtrType<int> sp_this(sp_from);
        EXPECT_EQ(sp_this.use_count(), 3);
        sp_this = sp_this;
        EXPECT_EQ(sp_this.use_count(), 3);
    }

    // Move assignment operator
    {
        // initially empty
        SharedPtrType<int> sp;
        sp = SharedPtrType<int>(new int(11));
        EXPECT_TRUE(sp);
        EXPECT_EQ(*sp, 11);
        EXPECT_EQ(sp.use_count(), 1);

        // release the previous pointer before assignment
        SharedPtrType<int> sp_release(sp);
        EXPECT_EQ(*sp_release, 11);
        EXPECT_EQ(sp_release.use_count(), 2);
        EXPECT_EQ(sp.use_count(), 2);
        SharedPtrType<int> sp_from(new int(22));
        sp_release = std::move(sp_from);
        EXPECT_EQ(*sp_release, 22);
        EXPECT_EQ(sp_release.use_count(), 1); // acquired
        EXPECT_EQ(sp.use_count(), 1); // released
        EXPECT_EQ(sp_from.use_count(), 0);

        // test that we don't acquire *this
        SharedPtrType<int> sp_this(sp);
        EXPECT_EQ(sp_this.use_count(), 2);
        sp_this = std::move(sp_this);
        EXPECT_EQ(sp_this.use_count(), 2);
    }
}

template <template<typename> class SharedPtrType>
void test_reset()
{
    // initially empty
    SharedPtrType<int> sp;
    sp.reset(new int(11));
    EXPECT_TRUE(sp);
    EXPECT_EQ(*sp, 11);
    EXPECT_EQ(sp.use_count(), 1);

    // release the previous pointer before resetting
    SharedPtrType<int> sp_release(sp);
    EXPECT_EQ(*sp_release, 11);
    EXPECT_EQ(sp_release.use_count(), 2);
    EXPECT_EQ(sp.use_count(), 2);
    sp_release.reset(new int(22));
    EXPECT_EQ(*sp_release, 22);
    EXPECT_EQ(sp_release.use_count(), 1);
    EXPECT_EQ(sp.use_count(), 1);
}

template <template<typename> class SharedPtrType>
void test_accessors()
{
    // operator*
    {
        auto sp = SharedPtrType<int>(new int(1));
        int& data_ref = *sp;
        data_ref = 2;
        EXPECT_EQ(*sp, 2);
    }

    // operator->
    {
        auto sp = SharedPtrType<std::string>(new std::string("test"));
        EXPECT_EQ(sp->size(), 4);
    }

    // operator bool
    {
        SharedPtrType<int> sp;
        EXPECT_FALSE(sp);
        sp = SharedPtrType<int>(new int(1));
        EXPECT_TRUE(sp);
    }
}


template <template<typename> class SharedPtrType>
void test_shared_ptr_interface()
{
    test_constructors<SharedPtrType>();
    test_assignment_operators<SharedPtrType>();
    test_reset<SharedPtrType>();
    test_accessors<SharedPtrType>();
}

// Assignment from T*
void test_si_specific_interface()
{
    // initially empty
    si::shared_ptr<int> sp;
    sp = new int(11);
    EXPECT_TRUE(sp);
    EXPECT_EQ(*sp, 11);
    EXPECT_EQ(sp.use_count(), 1);

    // release the previous pointer before assignment
    si::shared_ptr<int> sp_release(sp);
    EXPECT_EQ(*sp_release, 11);
    EXPECT_EQ(sp_release.use_count(), 2);
    EXPECT_EQ(sp.use_count(), 2);
    sp_release = new int(22);
    EXPECT_EQ(*sp_release, 22);
    EXPECT_EQ(sp_release.use_count(), 1);
    EXPECT_EQ(sp.use_count(), 1);
}


// Confirm the unit tests are correct.
TEST(std_shared_ptr, interface)
{
    test_shared_ptr_interface<std::shared_ptr>();
}

// Validate our implementation using the same tests.
TEST(si_shared_ptr, interface)
{
    test_shared_ptr_interface<si::shared_ptr>();
    test_si_specific_interface();
}

