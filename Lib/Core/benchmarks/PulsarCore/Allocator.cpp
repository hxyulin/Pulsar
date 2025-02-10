// NOLINTBEGIN(*)
#include "PulsarCore/GC/Allocators/Arena.hpp"

#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <vector>

using namespace Pulsar::GC;

// Test structure with some typical game object size
struct TestObject_t {
    float    m_position[3];
    float    m_rotation[4];
    float    m_scale[3];
    int32_t  m_flags;
    uint64_t m_id;
    // Add some padding to make it more realistic
    char m_name[32];
};

// Benchmark parameters
const std::vector<int> ALLOCATION_COUNTS = {100, 1000, 10000};

// Standard allocator benchmark
static void BM_StandardAllocator(benchmark::State& state) {
    const int                  N = state.range(0);
    std::vector<TestObject_t*> objects;
    objects.reserve(N);

    for (auto _ : state) {
        state.PauseTiming(); // Don't count cleanup time
        objects.clear();
        state.ResumeTiming();

        for (int i = 0; i < N; ++i) {
            auto* obj = new TestObject_t();
            objects.push_back(obj);
        }

        for (auto* ptr : objects) {
            delete ptr;
        }
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.SetBytesProcessed(state.iterations() * N * sizeof(TestObject_t));
}

// Arena allocator benchmark
static void BM_ArenaAllocator(benchmark::State& state) {
    const int                  N = state.range(0);
    std::vector<TestObject_t*> objects;
    objects.reserve(N);

    for (auto _ : state) {
        state.PauseTiming(); // Don't count cleanup/setup time
        objects.clear();
        ArenaAllocator<TestObject_t> arena(N * sizeof(TestObject_t));
        state.ResumeTiming();

        for (int i = 0; i < N; ++i) {
            auto* obj = std::allocator_traits<ArenaAllocator<TestObject_t>>::allocate(arena, 1);
            std::allocator_traits<ArenaAllocator<TestObject_t>>::construct(arena, obj);
            objects.push_back(obj);
        }

        for (auto* ptr : objects) {
            std::allocator_traits<ArenaAllocator<TestObject_t>>::destroy(arena, ptr);
            std::allocator_traits<ArenaAllocator<TestObject_t>>::deallocate(arena, ptr, 1);
        }
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.SetBytesProcessed(state.iterations() * N * sizeof(TestObject_t));
}

// STL allocator benchmark
static void BM_STLAllocator(benchmark::State& state) {
    const int                  N = state.range(0);
    std::vector<TestObject_t*> objects;
    objects.reserve(N);
    std::allocator<TestObject_t> alloc;

    for (auto _ : state) {
        state.PauseTiming();
        objects.clear();
        state.ResumeTiming();

        for (int i = 0; i < N; ++i) {
            auto* obj = std::allocator_traits<std::allocator<TestObject_t>>::allocate(alloc, 1);
            std::allocator_traits<std::allocator<TestObject_t>>::construct(alloc, obj);
            objects.push_back(obj);
        }

        for (auto* ptr : objects) {
            std::allocator_traits<std::allocator<TestObject_t>>::destroy(alloc, ptr);
            std::allocator_traits<std::allocator<TestObject_t>>::deallocate(alloc, ptr, 1);
        }
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.SetBytesProcessed(state.iterations() * N * sizeof(TestObject_t));
}

// Batch allocation benchmark for Arena
static void BM_ArenaAllocatorBatch(benchmark::State& state) {
    const int N = state.range(0);

    for (auto _ : state) {
        ArenaAllocator<TestObject_t> arena(N * sizeof(TestObject_t));
        auto* objects = std::allocator_traits<ArenaAllocator<TestObject_t>>::allocate(arena, N);

        // Construct all objects
        for (int i = 0; i < N; ++i) {
            std::allocator_traits<ArenaAllocator<TestObject_t>>::construct(arena, &objects[i]);
        }

        benchmark::DoNotOptimize(objects);

        // Destroy all objects
        for (int i = 0; i < N; ++i) {
            std::allocator_traits<ArenaAllocator<TestObject_t>>::destroy(arena, &objects[i]);
        }
        std::allocator_traits<ArenaAllocator<TestObject_t>>::deallocate(arena, objects, N);
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.SetBytesProcessed(state.iterations() * N * sizeof(TestObject_t));
}

// Different sized objects to test with
struct SmallObject {
    int x, y;  // 8 bytes
};

struct MediumObject {
    double position[3];  // 24 bytes
    int    flags;
    char   name[16];
};

struct LargeObject {
    double matrix[16];   // 128 bytes
    char   name[64];
    int    flags[8];
};

// Mixed size allocation benchmark
static void BM_MixedSizeAllocations(benchmark::State& state) {
    const int          N = state.range(0);
    std::vector<void*> objects;
    objects.reserve(N * 3); // For all three types

    // Start with base allocator
    ArenaAllocator<std::byte> baseArena(
        N * (sizeof(SmallObject) + sizeof(MediumObject) + sizeof(LargeObject)));

    // Rebind for each type
    typename std::allocator_traits<ArenaAllocator<std::byte>>::template rebind_alloc<SmallObject>
        smallAlloc(baseArena);
    typename std::allocator_traits<ArenaAllocator<std::byte>>::template rebind_alloc<MediumObject>
        mediumAlloc(baseArena);
    typename std::allocator_traits<ArenaAllocator<std::byte>>::template rebind_alloc<LargeObject>
        largeAlloc(baseArena);

    for (auto _ : state) {
        // Allocate in rotating pattern
        for (int i = 0; i < N; ++i) {
            auto* small  = std::allocator_traits<decltype(smallAlloc)>::allocate(smallAlloc, 1);
            auto* medium = std::allocator_traits<decltype(mediumAlloc)>::allocate(mediumAlloc, 1);
            auto* large  = std::allocator_traits<decltype(largeAlloc)>::allocate(largeAlloc, 1);

            objects.push_back(small);
            objects.push_back(medium);
            objects.push_back(large);
        }

        // Cleanup
        baseArena.reset();
        objects.clear();
    }

    state.SetItemsProcessed(state.iterations() * N * 3);
    state.SetBytesProcessed(state.iterations() * N
        * (sizeof(SmallObject) + sizeof(MediumObject) + sizeof(LargeObject)));
}

// Random access pattern benchmark
static void BM_RandomAccessPattern(benchmark::State& state) {
    const int                  N = state.range(0);
    std::vector<MediumObject*> objects;
    objects.reserve(N);

    std::mt19937     rng(42); // Fixed seed for reproducibility
    std::vector<int> access_pattern(N);
    std::iota(access_pattern.begin(), access_pattern.end(), 0);

    for (auto _ : state) {
        ArenaAllocator<MediumObject> arena(N * sizeof(MediumObject));

        // Allocate sequentially
        for (int i = 0; i < N; ++i) {
            auto* obj = std::allocator_traits<ArenaAllocator<MediumObject>>::allocate(arena, 1);
            std::allocator_traits<ArenaAllocator<MediumObject>>::construct(arena, obj);
            objects.push_back(obj);
        }

        // Access in random order
        std::shuffle(access_pattern.begin(), access_pattern.end(), rng);
        for (int idx : access_pattern) {
            benchmark::DoNotOptimize(*objects[idx]);
        }

        // Cleanup
        for (auto* ptr : objects) {
            std::allocator_traits<ArenaAllocator<MediumObject>>::destroy(arena, ptr);
            std::allocator_traits<ArenaAllocator<MediumObject>>::deallocate(arena, ptr, 1);
        }
        objects.clear();
    }

    state.SetItemsProcessed(state.iterations() * N * 2); // Allocation + access
    state.SetBytesProcessed(state.iterations() * N * sizeof(MediumObject));
}

// Fragmentation test (allocate/deallocate in specific pattern)
static void BM_FragmentationPattern(benchmark::State& state) {
    const int                  N = state.range(0);
    std::vector<MediumObject*> objects;
    objects.reserve(N);

    for (auto _ : state) {
        ArenaAllocator<MediumObject> arena(
            N * sizeof(MediumObject) * 2); // Extra space for fragmentation

        // First phase: allocate everything
        for (int i = 0; i < N; ++i) {
            auto* obj = std::allocator_traits<ArenaAllocator<MediumObject>>::allocate(arena, 1);
            std::allocator_traits<ArenaAllocator<MediumObject>>::construct(arena, obj);
            objects.push_back(obj);
        }

        // Second phase: deallocate every other object
        for (size_t i = 0; i < objects.size(); i += 2) {
            std::allocator_traits<ArenaAllocator<MediumObject>>::destroy(arena, objects[i]);
            std::allocator_traits<ArenaAllocator<MediumObject>>::deallocate(arena, objects[i], 1);
            objects[i] = nullptr;
        }

        // Third phase: try to allocate new objects
        for (size_t i = 0; i < objects.size(); i += 2) {
            auto* obj = std::allocator_traits<ArenaAllocator<MediumObject>>::allocate(arena, 1);
            std::allocator_traits<ArenaAllocator<MediumObject>>::construct(arena, obj);
            objects[i] = obj;
        }

        // Cleanup
        for (auto* ptr : objects) {
            if (ptr) {
                std::allocator_traits<ArenaAllocator<MediumObject>>::destroy(arena, ptr);
                std::allocator_traits<ArenaAllocator<MediumObject>>::deallocate(arena, ptr, 1);
            }
        }
        objects.clear();
    }

    state.SetItemsProcessed(state.iterations() * N * 3); // Initial + deallocate + reallocate
    state.SetBytesProcessed(state.iterations() * N * sizeof(MediumObject) * 3);
}

// Alignment stress test
template<size_t Alignment> static void BM_AlignmentTest(benchmark::State& state) {
    struct alignas(Alignment) AlignedObject {
        char data[32];
    };

    const int                   N = state.range(0);
    std::vector<AlignedObject*> objects;
    objects.reserve(N);

    for (auto _ : state) {
        ArenaAllocator<AlignedObject> arena(N * sizeof(AlignedObject));

        for (int i = 0; i < N; ++i) {
            auto* obj = std::allocator_traits<ArenaAllocator<AlignedObject>>::allocate(arena, 1);
            std::allocator_traits<ArenaAllocator<AlignedObject>>::construct(arena, obj);
            // Verify alignment
            assert(reinterpret_cast<std::uintptr_t>(obj) % Alignment == 0);
            objects.push_back(obj);
        }

        // Cleanup
        for (auto* ptr : objects) {
            std::allocator_traits<ArenaAllocator<AlignedObject>>::destroy(arena, ptr);
            std::allocator_traits<ArenaAllocator<AlignedObject>>::deallocate(arena, ptr, 1);
        }
        objects.clear();
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.SetBytesProcessed(state.iterations() * N * sizeof(AlignedObject));
}

// Register benchmarks
BENCHMARK(BM_MixedSizeAllocations)->Range(100, 10000);
BENCHMARK(BM_RandomAccessPattern)->Range(100, 10000);
BENCHMARK(BM_FragmentationPattern)->Range(100, 10000);
BENCHMARK(BM_AlignmentTest<8>)->Range(100, 10000)->Name("BM_AlignmentTest_8byte");
BENCHMARK(BM_AlignmentTest<16>)->Range(100, 10000)->Name("BM_AlignmentTest_16byte");
BENCHMARK(BM_AlignmentTest<32>)->Range(100, 10000)->Name("BM_AlignmentTest_32byte");

// Register benchmarks with different allocation counts
BENCHMARK(BM_StandardAllocator)->Range(100, 10000);
BENCHMARK(BM_ArenaAllocator)->Range(100, 10000);
BENCHMARK(BM_STLAllocator)->Range(100, 10000);
BENCHMARK(BM_ArenaAllocatorBatch)->Range(100, 10000);

BENCHMARK_MAIN();
// NOLINTEND(*)
