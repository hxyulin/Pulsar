#pragma once

// TODO: Maybe look into using private inheritance to make this more efficient (std::allocator)

#include <atomic>
#include <memory>
#include <utility>

namespace Pulsar::GC {
    template<typename T> using DefaultAllocator = std::allocator<T>;

    /// A smart pointer that will delete the object when it goes out of scope
    /// # Ownership
    /// The pointer will have unique ownership of the object
    template<typename T, typename Allocator = DefaultAllocator<T>> class Scoped {
        using AllocatorTraits = std::allocator_traits<Allocator>;
    public:
        /// Constructs an owned pointer that will be deleted when the Scoped object goes out of scope
        /// # Ownership
        /// The pointer will be deleted when the Scoped object goes out of scope
        explicit Scoped(T* ptr, Allocator allocator = Allocator())
            : m_Ptr(ptr), m_Allocator(allocator) {
        }

        // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
        Scoped(nullptr_t = nullptr, Allocator allocator = Allocator())
            : m_Ptr(nullptr), m_Allocator(allocator) {
        }

        ~Scoped() {
            AllocatorTraits::destroy(m_Allocator, m_Ptr);
            AllocatorTraits::deallocate(m_Allocator, m_Ptr, 1);
        }

        Scoped(const Scoped&)            = delete;
        Scoped& operator=(const Scoped&) = delete;

        Scoped(Scoped&& other) noexcept
            : m_Ptr(other.m_Ptr), m_Allocator(std::move(other.m_Allocator)) {
            other.m_Ptr = nullptr;
        }

        /// Moves the pointer from the other object
        /// # Ownership
        /// The pointer will have unique ownership of the object
        /// The other object will be set to null (same as std::move)
        ///
        /// @param other The other object to move the pointer from
        Scoped& operator=(Scoped&& other) noexcept {
            [[likely]] if (this != &other) {
                AllocatorTraits::destroy(m_Allocator, m_Ptr);
                AllocatorTraits::deallocate(m_Allocator, m_Ptr, 1);
                m_Ptr       = other.m_Ptr;
                m_Allocator = std::move(other.m_Allocator);
                other.m_Ptr = nullptr;
            }
            return *this;
        }

        [[nodiscard]] T& operator*() const {
            return *m_Ptr;
        }

        [[nodiscard]] T* get() {
            return m_Ptr;
        }

        [[nodiscard]] const T* get() const {
            return m_Ptr;
        }

        [[nodiscard]] T* operator->() {
            return m_Ptr;
        }

        const T* operator->() const {
            return m_Ptr;
        }

        /// Swaps the pointers of the two objects
        void swap(Scoped& other) noexcept {
            std::swap(m_Ptr, other.m_Ptr);
            std::swap(m_Allocator, other.m_Allocator);
        }

        /// Resets the pointer to the given value
        /// @param ptr The new pointer
        /// @param allocator The new allocator
        void reset(T* ptr = nullptr, Allocator allocator = Allocator()) {
            AllocatorTraits::destroy(m_Allocator, m_Ptr);
            AllocatorTraits::deallocate(m_Allocator, m_Ptr, 1);
            m_Ptr       = ptr;
            m_Allocator = allocator;
        }
    private:
        T*        m_Ptr;
        Allocator m_Allocator;
    };

    template<typename T, typename Allocator = DefaultAllocator<T>> class Weak;

    template<typename T, typename Allocator = DefaultAllocator<T>> class Ref {
        using AllocatorTraits = std::allocator_traits<Allocator>;
    public:
        friend class Weak<T, Allocator>;

        explicit Ref(T* ptr, Allocator allocator = Allocator())
            : m_Ptr(ptr), m_RefCount(new RefCount_t()), m_Allocator(allocator) {
        }

        // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
        Ref(nullptr_t = nullptr, Allocator allocator = Allocator())
            : m_Ptr(nullptr), m_RefCount(nullptr), m_Allocator(allocator) {
        }

        ~Ref() {
            cleanup();
        }

        Ref(const Ref& other) noexcept {
            if (other.m_RefCount == nullptr) {
                // After cleaning up, everything is null, so we can just return
                return;
            }

            other.m_RefCount->m_StrongCount += 1;
            m_RefCount  = other.m_RefCount;
            m_Ptr       = other.m_Ptr;
            m_Allocator = other.m_Allocator;
        }

        Ref& operator=(const Ref& other) noexcept {
            [[likely]] if (this != &other) {
                cleanup();
                if (other.m_RefCount == nullptr) {
                    return *this;
                }

                other.m_RefCount->m_StrongCount += 1;
                m_RefCount  = other.m_RefCount;
                m_Ptr       = other.m_Ptr;
                m_Allocator = other.m_Allocator;
            }

            return *this;
        }

        Ref(Ref&& other) noexcept
            : m_Ptr(other.m_Ptr), m_RefCount(other.m_RefCount),
              m_Allocator(std::move(other.m_Allocator)) {
            other.m_RefCount = nullptr;
            other.m_Ptr      = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            [[likely]] if (this != &other) {
                cleanup();
                if (other.m_RefCount == nullptr) {
                    return *this;
                }

                m_RefCount       = other.m_RefCount;
                other.m_RefCount = nullptr;
                m_Ptr            = other.m_Ptr;
                other.m_Ptr      = nullptr;
                m_Allocator      = std::move(other.m_Allocator);
            }

            return *this;
        }

        T& operator*() const {
            return *m_Ptr;
        }

        T* get() {
            return m_Ptr;
        }

        const T* get() const {
            return m_Ptr;
        }

        T* operator->() {
            return m_Ptr;
        }

        const T* operator->() const {
            return m_Ptr;
        }

    private:
        void cleanup() {
            if (m_RefCount == nullptr) {
                return;
            }

            if (--m_RefCount->m_StrongCount == 0) {
                AllocatorTraits::destroy(m_Allocator, m_Ptr);
                AllocatorTraits::deallocate(m_Allocator, m_Ptr, 1);
                m_Ptr = nullptr;

                // If the weak count is zero, then the object is no longer referenced by any weak pointers,
                // otherwise, the object is still referenced by some weak pointers, so we don't delete the RefCount
                // But the weak pointers will be no longer valid, (by checking that the strong count is zero)
                if (m_RefCount->m_WeakCount == 0) {
                    AllocatorTraits::destroy(m_Allocator, m_RefCount);
                    AllocatorTraits::deallocate(m_Allocator, m_Ptr, 1);
                }
                // So no matter what, we can set the RefCount to null to prevent use after free
                m_RefCount = nullptr;
            }
        }

        struct alignas(8) RefCount_t {
            std::atomic<uint32_t> m_StrongCount = std::atomic<uint32_t>(1);
            std::atomic<uint32_t> m_WeakCount   = std::atomic<uint32_t>(0);
        };

        T*          m_Ptr;
        RefCount_t* m_RefCount;
        Allocator   m_Allocator;
    };

    template<typename T, typename Allocator> class Weak {
    public:
    };

    template<typename T, typename Allocator = DefaultAllocator<T>, typename... Args>
    Scoped<T, Allocator> make_scoped(Args&&... args) {
        using AllocatorTraits = std::allocator_traits<Allocator>;
        Allocator alloc;
        auto*     ptr = AllocatorTraits::allocate(alloc, 1);
        AllocatorTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        return Scoped<T, Allocator>(ptr, alloc);
    }
} // namespace Pulsar::GC
