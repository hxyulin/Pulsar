// NOLINTBEGIN(*)
#include "PulsarCore/Types.hpp"

#include <PulsarCore/GC/Pointer.hpp>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>

using Pulsar::usize, Pulsar::i32;

class TrackingAllocatorInner {
public:
    TrackingAllocatorInner() = default;

    template<typename T> [[nodiscard]] T* allocate(usize n) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        T*                          ptr = std::allocator<T>().allocate(n);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        s_Allocations[ptr] = n;
        std::cerr << "allocating " << ptr << "\n";
        return ptr;
    }

    template<typename T> void deallocate(T* p, usize n) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        std::cerr << "deallocating " << p << "\n";
        auto iter = s_Allocations.find(p);
        EXPECT_TRUE(iter != s_Allocations.end()) << "Trying to deallocate untracked memory";
        EXPECT_EQ(iter->second, n) << "Deallocation size mismatch";
        s_Allocations.erase(iter);
        std::allocator<T>().deallocate(p, n);
    }

    static void assert_no_leaks() {
        std::lock_guard<std::mutex> lock(s_Mutex);
        EXPECT_TRUE(s_Allocations.empty()) << s_Allocations.size() << " memory leaks detected";
        if (!s_Allocations.empty()) {
            for (const auto& [ptr, size] : s_Allocations) {
                std::cerr << "Leak: " << ptr << " size: " << size << "\n";
            }
            s_Allocations.clear(); // Clear for next test
        }
    }

private:
    static inline std::mutex                             s_Mutex;
    static inline std::unordered_map<const void*, usize> s_Allocations;
};

// NOLINTNEXTLINE(misc-use-anonymous-namespace, cppcoreguidelines-avoid-non-const-global-variables)
static TrackingAllocatorInner g_TrackingAllocatorInner;

template<typename T> class TrackingAllocator {
public:
    using value_type                             = T;
    using size_type                              = usize;
    using difference_type                        = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    TrackingAllocator() = default;

    // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions, readability-named-parameter, hicpp-named-parameter)
    template<typename U> TrackingAllocator(const TrackingAllocator<U>&) {
    }

    [[nodiscard]] T* allocate(usize n) {
        return g_TrackingAllocatorInner.allocate<T>(n);
    }

    void deallocate(T* p, usize n) {
        if (p == nullptr) {
            return;
        }
        g_TrackingAllocatorInner.deallocate(p, n);
    }

    template<typename U, typename... Args> void construct(U* p, Args&&... args) {
        ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }

    template<typename U> void destroy(U* p) {
        p->~U();
    }

    template<typename U>
    // NOLINTNEXTLINE(readability-identifier-naming)
    struct rebind {
        using other = TrackingAllocator<U>;
    };
};

// NOLINTNEXTLINE(misc-use-anonymous-namespace)
template<typename T, typename... Args> [[nodiscard]] static T* allocate(Args&&... args) {
    TrackingAllocator<T> allocator {};
    T*                   ptr = allocator.allocate(1);
    allocator.construct(ptr, std::forward<Args>(args)...);
    return ptr;
}

// Helper namespace
namespace Test::GC {
    template<typename T> using Scoped = Pulsar::GC::Scoped<T, TrackingAllocator<T>>;

    template<typename T> using Ref = Pulsar::GC::Ref<T, TrackingAllocator<T>>;

    template<typename T> using Weak = Pulsar::GC::Weak<T, TrackingAllocator<T>>;
} // namespace Test::GC
using namespace Test;

class TestClass {
public:
    static i32 s_InstanceCount;

    TestClass() {
        s_InstanceCount++;
    }
    explicit TestClass(i32 i) : m_I(i) {
        s_InstanceCount++;
    }
    ~TestClass() {
        s_InstanceCount--;
    }
    TestClass(const TestClass&)            = delete;
    TestClass& operator=(const TestClass&) = delete;
    TestClass(TestClass&&)                 = delete;
    TestClass& operator=(TestClass&&)      = delete;

    [[nodiscard]] i32 get() const {
        return m_I.load(std::memory_order_relaxed);
    }

    void set(i32 i) {
        m_I.store(i, std::memory_order_relaxed);
    }

    void increment() {
        m_I.fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic<i32> m_I {0};
};


int TestClass::s_InstanceCount = 0;

// Existing Scoped tests plus additional edge cases
TEST(Pointer, Scoped) {
    // Basic functionality
    GC::Scoped<i32> scoped(allocate<i32>(5));
    EXPECT_EQ(*scoped, 5);

    // Custom class
    GC::Scoped<TestClass> testClassScoped(allocate<TestClass>(5));
    EXPECT_EQ(testClassScoped->get(), 5);

    // Reset to nullptr
    scoped.reset();
    EXPECT_EQ(scoped.get(), nullptr);
    testClassScoped.reset();
    EXPECT_EQ(testClassScoped.get(), nullptr);

    // Move constructor
    GC::Scoped<TestClass> original(allocate<TestClass>(10));
    GC::Scoped<TestClass> moved(std::move(original));
    EXPECT_EQ(moved->get(), 10);
    EXPECT_EQ(original.get(), nullptr);
    moved.reset();

    // Swap
    GC::Scoped<TestClass> ptrA(allocate<TestClass>(1));
    GC::Scoped<TestClass> ptrB(allocate<TestClass>(2));
    ptrA.swap(ptrB);
    EXPECT_EQ(ptrA->get(), 2);
    EXPECT_EQ(ptrB->get(), 1);

    ptrA.reset();
    ptrB.reset();

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, ScopedMemoryLeaks) {
    TestClass::s_InstanceCount = 0;
    { GC::Scoped<TestClass> scoped(allocate<TestClass>()); }
    EXPECT_EQ(TestClass::s_InstanceCount, 0);

    {
        GC::Scoped<TestClass> ptrA(allocate<TestClass>());
        GC::Scoped<TestClass> ptrB(allocate<TestClass>());
        ptrA.swap(ptrB);
    }
    EXPECT_EQ(TestClass::s_InstanceCount, 0);

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, Ref) {
    {
        // Basic functionality
        GC::Ref<TestClass> ref(allocate<TestClass>(5));
        EXPECT_EQ(ref->get(), 5);

        // Copy constructor
        GC::Ref<TestClass> copy(ref);
        EXPECT_EQ(copy->get(), 5);
        copy->set(10);
        EXPECT_EQ(ref->get(), 10); // Should affect both references

        // Move constructor
        GC::Ref<TestClass> moved(std::move(copy));
        EXPECT_EQ(moved->get(), 10);
        EXPECT_EQ(copy.get(), nullptr);

        // nullptr handling
        GC::Ref<TestClass> null(nullptr);
        EXPECT_EQ(null.get(), nullptr);

        // Copy assignment
        GC::Ref<TestClass> assigned = ref;
        EXPECT_EQ(assigned->get(), 10);

        // Move assignment
        GC::Ref<TestClass> moveAssigned = std::move(assigned);
        EXPECT_EQ(moveAssigned->get(), 10);
        EXPECT_EQ(assigned.get(), nullptr);
    }

    TrackingAllocatorInner::assert_no_leaks();
}


TEST(Pointer, RefThreadSafety) {
    {
        const i32 numThreads    = 4;
        const i32 numIterations = 1000;

        GC::Ref<TestClass>       shared(allocate<TestClass>(0));
        std::vector<std::thread> threads;

        threads.reserve(numThreads);
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&shared]() {
                for (int j = 0; j < numIterations; ++j) {
                    GC::Ref<TestClass> local = shared; // Copy constructor
                    local->increment();
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        EXPECT_EQ(shared->get(), numThreads * numIterations);
    }

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, RefEdgeCases) {
    {
        // Self-assignment
        GC::Ref<TestClass> self(allocate<TestClass>(1));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
        self = self;
#pragma clang diagnostic pop
        EXPECT_EQ(self->get(), 1);

        // Chain of moves
        GC::Ref<TestClass> ptrA(allocate<TestClass>(1));
        GC::Ref<TestClass> ptrB(std::move(ptrA));
        GC::Ref<TestClass> ptrC(std::move(ptrB));
        EXPECT_EQ(ptrC->get(), 1);
        EXPECT_EQ(ptrA.get(), nullptr);
        EXPECT_EQ(ptrB.get(), nullptr);

        // nullptr assignment
        GC::Ref<TestClass> notNull(allocate<TestClass>(1));
        notNull = nullptr;
        EXPECT_EQ(notNull.get(), nullptr);
    }

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, RefMemoryLeaks) {
    TestClass::s_InstanceCount = 0;
    {
        GC::Ref<TestClass> ref(allocate<TestClass>());
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        { GC::Ref<TestClass> copy = ref; }
        EXPECT_EQ(TestClass::s_InstanceCount, 1);
    }
    EXPECT_EQ(TestClass::s_InstanceCount, 0);

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakBasics) {
    TestClass::s_InstanceCount = 0;
    {
        // Basic weak pointer creation
        GC::Ref<TestClass>  ref(allocate<TestClass>(5));
        GC::Weak<TestClass> weak(ref);

        EXPECT_TRUE(weak.is_valid());

        // Test promotion to strong reference
        if (auto promoted = weak.lock()) {
            EXPECT_EQ(promoted.value()->get(), 5);
        }
        else {
            FAIL() << "Failed to lock valid weak pointer";
        }
    }
    // After ref is destroyed, weak should be invalid
    EXPECT_EQ(TestClass::s_InstanceCount, 0);

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakCopyAndMove) {
    {
        GC::Ref<TestClass>  ref(allocate<TestClass>(1));
        GC::Weak<TestClass> weak1(ref);

        // Copy constructor
        GC::Weak<TestClass> weak2(weak1);
        EXPECT_TRUE(weak2.is_valid());

        // Move constructor
        GC::Weak<TestClass> weak3(std::move(weak1));
        EXPECT_TRUE(weak3.is_valid());

        // Copy assignment
        GC::Weak<TestClass> weak4(ref);
        weak4 = weak2;
        EXPECT_TRUE(weak4.is_valid());

        // Move assignment
        GC::Weak<TestClass> weak5(ref);
        weak5 = std::move(weak3);
        EXPECT_TRUE(weak5.is_valid());

        // Test that moving invalidates the source
        if (auto promoted = weak1.lock()) {
            FAIL() << "Moved-from weak pointer should not be promoteable";
        }
    }

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakExpiration) {
    GC::Weak<TestClass> weak;
    {
        GC::Ref<TestClass> ref(allocate<TestClass>(1));
        weak = GC::Weak<TestClass>(ref);
        EXPECT_TRUE(weak.is_valid());

        // Ref count should prevent destruction
        if (auto promoted = weak.lock()) {
            EXPECT_EQ(promoted.value()->get(), 1);
        }
        else {
            FAIL() << "Failed to promote valid weak pointer";
        }
    }
    EXPECT_FALSE(weak.is_valid());
    weak.reset();

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakThreadSafety) {
    const int numThreads    = 4;
    const int numIterations = 1000;

    GC::Ref<TestClass>       ref(allocate<TestClass>(0));
    GC::Weak<TestClass>      weak(ref);
    std::vector<std::thread> threads;

    // Test concurrent weak pointer operations
    threads.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&weak]() {
            for (int j = 0; j < numIterations; ++j) {
                if (auto promoted = weak.lock()) {
                    promoted.value()->increment();
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(weak.is_valid());
    if (auto final = weak.lock()) {
        EXPECT_EQ(final.value()->get(), numThreads * numIterations);
    }
    weak.reset();
    ref.reset();

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakCircularDependency) {
    struct Node_t {
        GC::Ref<Node_t>  m_next_strong;
        GC::Weak<Node_t> m_next_weak;
        explicit Node_t(int v) : m_value(v) {
        }
        int m_value;
    };

    {
        // Create circular reference with one weak pointer to break cycle
        GC::Ref<Node_t> node1(allocate<Node_t>(1));
        GC::Ref<Node_t> node2(allocate<Node_t>(2));

        node1->m_next_strong = node2;
        node2->m_next_weak   = GC::Weak<Node_t>(node1);

        // Verify circular reference
        EXPECT_EQ(node1->m_next_strong->m_value, 2);
        if (auto promoted = node2->m_next_weak.lock()) {
            EXPECT_EQ(promoted.value()->m_value, 1);
        }
        else {
            FAIL() << "Failed to promote valid weak pointer";
        }
    }

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, RefStressTest) {
    constexpr int NUM_THREADS    = 8;
    constexpr int NUM_ITERATIONS = 10000;

    GC::Ref<TestClass>       ref(allocate<TestClass>(0));
    std::vector<std::thread> threads;

    // Multiple threads creating and destroying references
    threads.reserve(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&ref]() {
            for (int j = 0; j < NUM_ITERATIONS; ++j) {
                GC::Ref<TestClass> localRef   = ref; // Create reference
                GC::Ref<TestClass> anotherRef = localRef; // Create another reference
                localRef.reset(); // Destroy one reference
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(ref.strong_ref_count(), 1); // Only original reference should remain
    ref.reset();
    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakPromotionRace) {
    constexpr int NUM_THREADS    = 8;
    constexpr int NUM_ITERATIONS = 1000;

    GC::Weak<TestClass> weak;
    {
        GC::Ref<TestClass> ref(allocate<TestClass>(0));
        weak = GC::Weak<TestClass>(ref);

        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);

        // Have multiple threads try to promote the weak reference while
        // the original reference still exists
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([&weak]() {
                for (int j = 0; j < NUM_ITERATIONS; ++j) {
                    if (auto promoted = weak.lock()) {
                        promoted.value()->increment();
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        EXPECT_TRUE(weak.is_valid());
        if (auto final = weak.lock()) {
            EXPECT_EQ(final.value()->get(), NUM_THREADS * NUM_ITERATIONS);
        }
    }
    // ref is now destroyed
    EXPECT_FALSE(weak.is_valid());
    weak.reset();

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakDestroyWhileLocking) {
    constexpr int NUMBER_OF_ITERATIONS = 1000;

    for (int i = 0; i < NUMBER_OF_ITERATIONS; ++i) {
        GC::Weak<TestClass> weak;
        {
            GC::Ref<TestClass> ref(allocate<TestClass>(0));
            weak = GC::Weak<TestClass>(ref);

            std::atomic<bool> keepRunning {true};

            // Have locker check keepRunning flag
            std::thread locker([&weak, &keepRunning]() {
                while (keepRunning) {
                    if (auto promoted = weak.lock()) {
                        promoted.value()->increment();
                    }
                }
            });

            // Give locker thread time to run
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // Signal locker to stop before destroying reference
            keepRunning = false;
            locker.join();
        } // ref destroyed here after locker is done

        EXPECT_FALSE(weak.is_valid());
        weak.reset();
    }

    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, RefCopyMoveStress) {
    constexpr int NUMBER_OF_ITERATIONS = 10000;

    TestClass::s_InstanceCount = 0;
    {
        GC::Ref<TestClass> original(allocate<TestClass>(1));

        // Use fixed array instead of vector to avoid reallocation issues
        GC::Ref<TestClass> temp1(nullptr);
        GC::Ref<TestClass> temp2(nullptr);

        for (int i = 0; i < NUMBER_OF_ITERATIONS; ++i) {
            temp1 = original; // Copy
            temp2 = std::move(temp1); // Move
            EXPECT_EQ(temp2->get(), 1);
            temp2.reset(); // Clean up
        }

        EXPECT_EQ(original.strong_ref_count(), 1);
        EXPECT_EQ(original->get(), 1);
    }
    EXPECT_EQ(TestClass::s_InstanceCount, 0);
    TrackingAllocatorInner::assert_no_leaks();
}

TEST(Pointer, WeakCopyMoveStress) {
    constexpr int NUM_ITERATIONS = 10000;

    GC::Ref<TestClass>               ref(allocate<TestClass>(1));
    std::vector<GC::Weak<TestClass>> weaks;

    // Test rapid weak pointer copy/move operations
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        GC::Weak<TestClass> weak(ref);
        weaks.push_back(weak); // Copy
        weaks.push_back(std::move(weak)); // Move

        // Verify that moved-from weak pointer is invalid
        EXPECT_FALSE(weak.is_valid());

        // Verify that copies are valid
        EXPECT_TRUE(weaks.back().is_valid());
        if (auto promoted = weaks.back().lock()) {
            EXPECT_EQ(promoted.value()->get(), 1);
        }

        weaks.clear(); // Destroy all weak pointers
    }

    EXPECT_EQ(ref.strong_ref_count(), 1);
    EXPECT_EQ(ref.weak_ref_count(), 0);

    ref.reset();
    weaks.clear();
    TrackingAllocatorInner::assert_no_leaks();
}
// NOLINTEND(*)
