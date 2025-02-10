#pragma once

#include "PulsarCore/GC/Pointer.hpp"
#include "PulsarCore/Types.hpp"
#include "PulsarCore/Util/Macros.hpp"

namespace Pulsar::GC {
    struct ArenaRegion_t {
        std::byte* m_Begin;
        usize      m_Size;
        usize      m_Allocated = 0;
#ifdef PULSAR_DEBUG
        // Used for debugging, to ensure that we don't reset the arena when we still have allocations
        usize m_AllocationCount = 0;
#endif

        explicit ArenaRegion_t(usize size) : m_Begin(new std::byte[size]), m_Size(size) {
            // TODO: for debugging, we might want to fill the memory with a known value to easily detect
            //       uninitialized memory
        }

        ~ArenaRegion_t() {
#ifdef PULSAR_DEBUG
            PULSAR_ASSERT(m_AllocationCount == 0,
                "There are still allocations in the arena when the arena is destroyed");
#endif
            delete[] m_Begin;
        }
        ArenaRegion_t(const ArenaRegion_t&)            = delete;
        ArenaRegion_t& operator=(const ArenaRegion_t&) = delete;
        ArenaRegion_t(ArenaRegion_t&&)                 = delete;
        ArenaRegion_t& operator=(ArenaRegion_t&&)      = delete;
    };

    template<typename T> class ArenaAllocator {
        template<typename U> friend class ArenaAllocator;
    public:
        using value_type                             = T;
        using size_type                              = usize;
        using difference_type                        = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::false_type;
        using is_always_equal                        = std::false_type;

        explicit ArenaAllocator(usize size = 1024UL * 1024UL)
            : m_Region(make_ref<ArenaRegion_t>(size)) {
        }

        ~ArenaAllocator() {
            m_Region.reset();
        }

        template<typename U>
        // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
        ArenaAllocator(const ArenaAllocator<U>& other) : m_Region(other.m_Region) {
        }

        ArenaAllocator(const ArenaAllocator&)            = default;
        ArenaAllocator& operator=(const ArenaAllocator&) = default;
        // FIXME: We may want to make this thread safe, and implement move assignment, but for now we don't want to check for nullptr
        ArenaAllocator(ArenaAllocator&&)            = default;
        ArenaAllocator& operator=(ArenaAllocator&&) = default;

        [[nodiscard]] T* allocate(usize count) {
            // TODO: Handle alignment
            auto size = count * sizeof(T);
            if (m_Region->m_Allocated + size > m_Region->m_Size) {
                throw std::bad_alloc();
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto* ptr = reinterpret_cast<T*>(m_Region->m_Begin + m_Region->m_Allocated);
            m_Region->m_Allocated += size;
#ifdef PULSAR_DEBUG
            m_Region->m_AllocationCount++;
#endif
            return ptr;
        }

        void deallocate(T* ptr, usize size) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto* ptr2 = reinterpret_cast<std::byte*>(ptr);
            m_Region->m_Allocated -= size;
            PULSAR_UNUSED(ptr2);
#ifdef PULSAR_DEBUG
            m_Region->m_AllocationCount--;
#endif
        }

        [[nodiscard]] usize max_size() const {
            return m_Region->m_Size;
        }

        [[nodiscard]] usize used_size() const {
            return m_Region->m_Allocated;
        }

        [[nodiscard]] usize available_size() const {
            return m_Region->m_Size - m_Region->m_Allocated;
        }

        void reset() {
            PULSAR_ASSERT(
                m_Region->m_AllocationCount == 0, "There are still allocations in the arena");
            m_Region->m_Allocated = 0;
        }

        template<typename U>
        // NOLINTNEXTLINE(readability-identifier-naming)
        struct rebind {
            using other = ArenaAllocator<U>;
        };

#ifdef PULSAR_DEBUG
        /// Returns the number of allocations that have been made in the arena
        /// This is only available in debug builds
        [[nodiscard]] usize allocation_count() const {
            return m_Region->m_AllocationCount;
        }
#endif

    private:
        Ref<ArenaRegion_t> m_Region;
    };

    // TODO: Implement a dynamic arena allocator / arena pool where it allocates arenas on demand
} // namespace Pulsar::GC
