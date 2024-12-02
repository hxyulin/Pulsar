#include <PulsarCore/GC/Pointer.hpp>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>


class TrackingAllocatorInner {
public:
    TrackingAllocatorInner() = default;

    template<typename T> [[nodiscard]] T* allocate(std::size_t n) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        T*                          ptr = std::allocator<T>().allocate(n);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        s_Allocations[ptr] = n;

        //std::cerr << "allocating " << ptr << "\n";
        return ptr;
    }

    template<typename T> void deallocate(T* p, std::size_t n) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        auto                        iter = s_Allocations.find(p);
        EXPECT_TRUE(iter != s_Allocations.end()) << "Trying to deallocate untracked memory";
        EXPECT_EQ(iter->second, n) << "Deallocation size mismatch";
        s_Allocations.erase(iter);
        std::allocator<T>().deallocate(p, n);
        //std::cerr << "deallocating " << p << "\n";
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
    static inline std::mutex                                   s_Mutex;
    static inline std::unordered_map<const void*, std::size_t> s_Allocations;
};

template<typename T> class TrackingAllocator {
public:
    using value_type                             = T;
    using size_type                              = std::size_t;
    using difference_type                        = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;

    [[nodiscard]] T* allocate(std::size_t n) {
        return s_Inner.allocate<T>(n);
    }

    void deallocate(T* p, std::size_t n) {
        if (p == nullptr) {
            return;
        }
        s_Inner.deallocate(p, n);
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

private:
    inline static TrackingAllocatorInner s_Inner;
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

    // NOLINTNEXTLINE(misc-use-anonymous-namespace)
    template<typename T, typename... Args> static Scoped<T> make_scoped(Args&&... args) {
        TrackingAllocator<T> alloc;
        T*                   ptr = alloc.allocate(1);
        alloc.construct(ptr, std::forward<Args>(args)...);
        return Scoped<T>(ptr, alloc);
    }
} // namespace Test::GC
using namespace Test;

class TestClass {
public:
    static int s_InstanceCount;

    TestClass() {
        s_InstanceCount++;
    }
    explicit TestClass(int i) : m_I(i) {
        s_InstanceCount++;
    }
    ~TestClass() {
        s_InstanceCount--;
    }
    TestClass(const TestClass&)            = delete;
    TestClass& operator=(const TestClass&) = delete;
    TestClass(TestClass&&)                 = delete;
    TestClass& operator=(TestClass&&)      = delete;

    [[nodiscard]] int get() const {
        return m_I.load(std::memory_order_relaxed);
    }

    void set(int i) {
        m_I.store(i, std::memory_order_relaxed);
    }

    void increment() {
        m_I.fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic<int> m_I {0};
};


int TestClass::s_InstanceCount = 0;

// Existing Scoped tests plus additional edge cases
TEST(Pointer, Scoped) {
        // Basic functionality
    GC::Scoped<int> scoped(allocate<int>(5));
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
        EXPECT_EQ(ref->get(), 10);  // Should affect both references

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

// We need to implement the test once we implement weak_pointers
/*TEST(Pointer, RefCircularDependency) {*/
/*    {*/
/*        struct alignas(32) Node_t {*/
/*            GC::Ref<Node_t> m_next;*/
/*            explicit Node_t(int v) : m_value(v) {*/
/*            }*/
/*            int m_value;*/
/*        };*/
/**/
/*        TestClass::s_InstanceCount = 0;*/
/*        {*/
/*            // Create circular reference*/
/*            GC::Ref<Node_t> node1(allocate<Node_t>(1));*/
/*            GC::Ref<Node_t> node2(allocate<Node_t>(2));*/
/*            node1->m_next = node2;*/
/*            node2->m_next = node1;*/
/**/
/*            // Verify circular reference*/
/*            EXPECT_EQ(node1->m_next->m_value, 2);*/
/*            EXPECT_EQ(node2->m_next->m_value, 1);*/
/*        }*/
/*        // Objects should be cleaned up despite circular reference*/
/*    }*/
/**/
/*    TrackingAllocatorInner::assert_no_leaks();*/
/*}*/

TEST(Pointer, RefThreadSafety) {
    {
        const int numThreads    = 4;
        const int numIterations = 1000;

        GC::Ref<TestClass>       shared(allocate<TestClass>(0));
        std::vector<std::thread> threads;

        threads.reserve(numThreads);
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&shared]() {
                for (int j = 0; j < numIterations; ++j) {
                    GC::Ref<TestClass> local = shared;  // Copy constructor
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
