// NOLINTBEGIN(*)
#include <gtest/gtest.h>
#define PULSAR_FORCE_DEBUG
#define PULSAR_ASSERT(expr, ...) EXPECT_TRUE(expr)
#include "PulsarCore/GC/Allocators/Arena.hpp"
#include "PulsarCore/GC/Pointer.hpp"
#include "PulsarCore/Util/Macros.hpp"

using namespace Pulsar::GC;

// Test struct to track constructions/destructions
struct TestObject_t {
    static size_t s_Constructions;
    static size_t s_Destructions;

    int m_value;

    explicit TestObject_t(int v = 0) : m_value(v) {
        ++s_Constructions;
    }
    ~TestObject_t() {
        ++s_Destructions;
    }
    TestObject_t(const TestObject_t&)            = default;
    TestObject_t& operator=(const TestObject_t&) = default;
    TestObject_t(TestObject_t&&)                 = default;
    TestObject_t& operator=(TestObject_t&&)      = default;
};

size_t TestObject_t::s_Constructions = 0;
size_t TestObject_t::s_Destructions  = 0;

class ArenaAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestObject_t::s_Constructions = 0;
        TestObject_t::s_Destructions  = 0;
    }
};

// Basic allocator conformance tests
TEST_F(ArenaAllocatorTest, AllocatorTraitsConformance) {
    using T      = TestObject_t;
    using Alloc  = ArenaAllocator<T>;
    using Traits = std::allocator_traits<Alloc>;

    // Type checks
    static_assert(std::is_same_v<typename Traits::value_type, T>);
    static_assert(std::is_same_v<typename Traits::pointer, T*>);
    static_assert(std::is_same_v<typename Traits::size_type, std::size_t>);

    // Rebind check
    static_assert(std::is_same_v<typename Traits::rebind_alloc<int>, ArenaAllocator<int>>);
}

// Test basic allocation and deallocation
TEST_F(ArenaAllocatorTest, BasicAllocation) {
    ArenaAllocator<TestObject_t> alloc(1024);
    auto* ptr = std::allocator_traits<ArenaAllocator<TestObject_t>>::allocate(alloc, 1);
    ASSERT_NE(ptr, nullptr);

    std::allocator_traits<ArenaAllocator<TestObject_t>>::construct(alloc, ptr, 42);
    EXPECT_EQ(ptr->m_value, 42);
    EXPECT_EQ(TestObject_t::s_Constructions, 1);

    std::allocator_traits<ArenaAllocator<TestObject_t>>::destroy(alloc, ptr);
    std::allocator_traits<ArenaAllocator<TestObject_t>>::deallocate(alloc, ptr, 1);
    EXPECT_EQ(TestObject_t::s_Destructions, 1);
}

// Test Scoped pointer with ArenaAllocator
TEST_F(ArenaAllocatorTest, ScopedPointerWithArena) {
    {
        auto ptr = make_scoped<TestObject_t, ArenaAllocator<TestObject_t>>(42);
        EXPECT_EQ(ptr->m_value, 42);
        EXPECT_EQ(TestObject_t::s_Constructions, 1);
    }
    EXPECT_EQ(TestObject_t::s_Destructions, 1);
}

// Test Ref pointer with ArenaAllocator
TEST_F(ArenaAllocatorTest, RefPointerWithArena) {
    Weak<TestObject_t, ArenaAllocator<TestObject_t>> weak;
    {
        auto ref = make_ref<TestObject_t, ArenaAllocator<TestObject_t>>(42);
        EXPECT_EQ(ref->m_value, 42);
        EXPECT_EQ(ref.strong_ref_count(), 1);

        weak = Weak(ref);
        EXPECT_EQ(ref.weak_ref_count(), 1);

        // Test ref counting
        {
            auto ref2 = ref;
            EXPECT_EQ(ref.strong_ref_count(), 2);
            PULSAR_UNUSED(ref2);
        }
        EXPECT_EQ(ref.strong_ref_count(), 1);
    }

    // Test weak pointer behavior
    EXPECT_FALSE(weak.is_valid());
    auto locked = weak.lock();
    EXPECT_FALSE(locked.has_value());
}

// Test multiple allocations and arena reset
TEST_F(ArenaAllocatorTest, MultipleAllocationsAndReset) {
    ArenaAllocator<TestObject_t> alloc(1024);

    std::vector<TestObject_t*, std::allocator<TestObject_t*>> pointers;
    for (int i = 0; i < 10; i++) {
        auto* ptr = std::allocator_traits<ArenaAllocator<TestObject_t>>::allocate(alloc, 1);
        std::allocator_traits<ArenaAllocator<TestObject_t>>::construct(alloc, ptr, i);
        pointers.push_back(ptr);
    }

    EXPECT_EQ(TestObject_t::s_Constructions, 10);

    // Cleanup
    for (auto* ptr : pointers) {
        std::allocator_traits<ArenaAllocator<TestObject_t>>::destroy(alloc, ptr);
        std::allocator_traits<ArenaAllocator<TestObject_t>>::deallocate(alloc, ptr, 1);
    }

    EXPECT_EQ(TestObject_t::s_Destructions, 10);
    EXPECT_NO_THROW(alloc.reset());
}

// Test allocator rebinding
TEST_F(ArenaAllocatorTest, AllocatorRebinding) {
    ArenaAllocator<TestObject_t> alloc(1024);
    typename std::allocator_traits<ArenaAllocator<TestObject_t>>::template rebind_alloc<int>
        intAlloc(alloc);

    auto* intPtr = std::allocator_traits<decltype(intAlloc)>::allocate(intAlloc, 1);
    ASSERT_NE(intPtr, nullptr);
    std::allocator_traits<decltype(intAlloc)>::construct(intAlloc, intPtr, 42);
    EXPECT_EQ(*intPtr, 42);

    std::allocator_traits<decltype(intAlloc)>::destroy(intAlloc, intPtr);
    std::allocator_traits<decltype(intAlloc)>::deallocate(intAlloc, intPtr, 1);
}

TEST_F(ArenaAllocatorTest, ArenaReuseAfterReset) {
    ArenaAllocator<TestObject_t> alloc(1024);
    constexpr int                kIterations          = 5;
    constexpr int                kObjectsPerIteration = 10;

    for (int iter = 0; iter < kIterations; ++iter) {
        // Allocate and construct objects
        std::vector<TestObject_t*> objects;
        for (int i = 0; i < kObjectsPerIteration; ++i) {
            auto* ptr = std::allocator_traits<ArenaAllocator<TestObject_t>>::allocate(alloc, 1);
            std::allocator_traits<ArenaAllocator<TestObject_t>>::construct(alloc, ptr, i);
            EXPECT_EQ(ptr->m_value, i)
                << "Value mismatch in iteration " << iter << ", object " << i;
            objects.push_back(ptr);
        }

        // Verify all objects
        for (int i = 0; i < kObjectsPerIteration; ++i) {
            EXPECT_EQ(objects[i]->m_value, i)
                << "Value corrupted in iteration " << iter << ", object " << i;
        }

        // Cleanup objects
        for (auto* ptr : objects) {
            std::allocator_traits<ArenaAllocator<TestObject_t>>::destroy(alloc, ptr);
            std::allocator_traits<ArenaAllocator<TestObject_t>>::deallocate(alloc, ptr, 1);
        }

        // Reset arena for next iteration
        alloc.reset();

        // Verify construction/destruction counts
        EXPECT_EQ(TestObject_t::s_Constructions, (iter + 1) * kObjectsPerIteration)
            << "Incorrect number of constructions in iteration " << iter;
        EXPECT_EQ(TestObject_t::s_Destructions, (iter + 1) * kObjectsPerIteration)
            << "Incorrect number of destructions in iteration " << iter;
    }

    // Final verification
    EXPECT_EQ(TestObject_t::s_Constructions, kIterations * kObjectsPerIteration);
    EXPECT_EQ(TestObject_t::s_Destructions, kIterations * kObjectsPerIteration);
}

// NOLINTEND(*)
