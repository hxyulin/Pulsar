#pragma once

// TODO: Maybe look into using private inheritance to make this more efficient (std::allocator)

#include "PulsarCore/Types.hpp"

#include <atomic>
#include <memory>
#include <optional>
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
        Scoped(std::nullptr_t = nullptr, Allocator allocator = Allocator())
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

        bool operator==(const Scoped& other) const {
            return m_Ptr == other.m_Ptr;
        }

        bool operator==(std::nullptr_t) const {
            return m_Ptr == nullptr;
        }

    private:
        T*        m_Ptr;
        Allocator m_Allocator;
    };

    // TODO: Make thread safe using atomic
    struct alignas(8) RefCount_t {
        std::atomic<u32> m_StrongCount = 1;
        std::atomic<u32> m_WeakCount   = 0;
    };

    template<typename T, typename Allocator = DefaultAllocator<T>> class Weak;

    template<typename T, typename Allocator = DefaultAllocator<T>> class Ref {
    public:
        using AllocatorTraits = std::allocator_traits<Allocator>;
        using RcAllocator =
            typename std::allocator_traits<Allocator>::template rebind_alloc<RefCount_t>;
        using RcAllocatorTraits = std::allocator_traits<RcAllocator>;
        friend class Weak<T, Allocator>;

        explicit Ref(T* ptr, Allocator allocator = Allocator())
            : m_Ptr(ptr), m_RefCount(nullptr), m_Allocator(allocator) {
            RcAllocator rcAlloc(m_Allocator);
            m_RefCount = RcAllocatorTraits::allocate(rcAlloc, 1);
            RcAllocatorTraits::construct(rcAlloc, m_RefCount);
        }

        // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
        Ref(std::nullptr_t = nullptr, Allocator allocator = Allocator())
            : m_Ptr(nullptr), m_RefCount(nullptr), m_Allocator(allocator) {
        }

        ~Ref() {
            reset();
        }

        Ref(const Ref& other) noexcept {
            if (other.m_RefCount == nullptr) {
                m_RefCount  = nullptr;
                m_Ptr       = nullptr;
                m_Allocator = other.m_Allocator;
                return;
            }

            other.m_RefCount->m_StrongCount += 1;
            m_RefCount  = other.m_RefCount;
            m_Ptr       = other.m_Ptr;
            m_Allocator = other.m_Allocator;
        }

        Ref& operator=(const Ref& other) noexcept {
            [[likely]] if (this != &other) {
                reset();
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
                reset();
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

        [[nodiscard]] u32 strong_ref_count() const {
            return m_RefCount->m_StrongCount;
        }

        [[nodiscard]] u32 weak_ref_count() const {
            return m_RefCount->m_WeakCount;
        }

        bool operator==(const Ref& other) const {
            return m_Ptr == other.m_Ptr;
        }

        bool operator==(std::nullptr_t) const {
            return m_Ptr == nullptr;
        }

        void reset() {
            if (m_RefCount == nullptr) {
                return;
            }

            if (--m_RefCount->m_StrongCount == 0) {
                // We're the last strong reference, delete the object
                AllocatorTraits::destroy(m_Allocator, m_Ptr);
                AllocatorTraits::deallocate(m_Allocator, m_Ptr, 1);
                m_Ptr = nullptr;

                // Now handle the RefCount cleanup
                if (m_RefCount->m_WeakCount == 0) {
                    RcAllocator rcAlloc(m_Allocator);
                    RcAllocatorTraits::destroy(rcAlloc, m_RefCount);
                    RcAllocatorTraits::deallocate(rcAlloc, m_RefCount, 1);
                }
            }
            m_RefCount = nullptr;
        }

    private:
        Ref(T* ptr, RefCount_t* refCount, Allocator allocator) noexcept
            : m_Ptr(ptr), m_RefCount(refCount), m_Allocator(allocator) {
            if (m_RefCount != nullptr) {
                m_RefCount->m_StrongCount++;
            }
        }

        T*          m_Ptr;
        RefCount_t* m_RefCount;
        Allocator   m_Allocator;
    };

    template<typename T, typename Allocator> class Weak {
    public:
        using AllocatorTraits = std::allocator_traits<Allocator>;
        using RcAllocator =
            typename std::allocator_traits<Allocator>::template rebind_alloc<RefCount_t>;
        using RcAllocatorTraits = std::allocator_traits<RcAllocator>;

        explicit Weak(Ref<T, Allocator>& ref)
            : m_Ptr(ref.m_Ptr), m_RefCount(ref.m_RefCount), m_Allocator(ref.m_Allocator) {
            if (m_RefCount != nullptr) {
                m_RefCount->m_WeakCount++;
            }
        }

        // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
        Weak(std::nullptr_t = nullptr, Allocator allocator = Allocator())
            : m_Ptr(nullptr), m_RefCount(nullptr), m_Allocator(allocator) {
        }

        ~Weak() {
            reset();
        }
        Weak(const Weak& other)
            : m_Ptr(other.m_Ptr), m_RefCount(other.m_RefCount), m_Allocator(other.m_Allocator) {
            if (m_RefCount != nullptr) {
                m_RefCount->m_WeakCount++;
            }
        }

        Weak& operator=(const Weak& other) {
            if (this != &other) {
                reset();
                m_RefCount  = other.m_RefCount;
                m_Ptr       = other.m_Ptr;
                m_Allocator = other.m_Allocator;
                if (m_RefCount != nullptr) {
                    m_RefCount->m_WeakCount++;
                }
            }
            return *this;
        }

        Weak(Weak&& other) noexcept
            : m_Ptr(other.m_Ptr), m_RefCount(other.m_RefCount),
              m_Allocator(std::move(other.m_Allocator)) {
            other.m_Ptr      = nullptr;
            other.m_RefCount = nullptr;
        }

        Weak& operator=(Weak&& other) noexcept {
            if (this != &other) {
                reset();
                m_RefCount       = other.m_RefCount;
                m_Ptr            = other.m_Ptr;
                other.m_Ptr      = nullptr;
                other.m_RefCount = nullptr;
                m_Allocator      = std::move(other.m_Allocator);
            }
            return *this;
        }

        [[nodiscard]] u32 strong_ref_count() const {
            return m_RefCount->m_StrongCount;
        }

        [[nodiscard]] u32 weak_ref_count() const {
            return m_RefCount->m_WeakCount;
        }


        /// Checks if the weak pointer is valid
        [[nodiscard]] bool is_valid() const {
            return m_RefCount != nullptr && m_RefCount->m_StrongCount != 0;
        }

        [[nodiscard]] std::optional<Ref<T, Allocator>> lock() const {
            if (!is_valid()) {
                return std::nullopt;
            }
            // We need to increment the strong count here to prevent a race condition
            m_RefCount->m_StrongCount++;
            auto ref = Ref<T, Allocator>(m_Ptr, m_RefCount, m_Allocator);
            // After the lock is acquired, we need to decrement the strong count
            // FIXME: We may want to check that the strong count is the same here, because another thread could have
            //        acquired the lock before we decremented the strong count. We could use a compare_exchange_strong
            m_RefCount->m_StrongCount--;
            return ref;
        }

        void reset() {
            if (m_RefCount == nullptr) {
                return;
            }

            // If this is the last weak reference and there are no strong references
            if (--m_RefCount->m_WeakCount == 0) {
                // Only delete RefCount if there are no strong references
                if (m_RefCount->m_StrongCount == 0) {
                    RcAllocator rcAlloc(m_Allocator);
                    RcAllocatorTraits::destroy(rcAlloc, m_RefCount);
                    RcAllocatorTraits::deallocate(rcAlloc, m_RefCount, 1);
                }
            }
            m_RefCount = nullptr;
            m_Ptr      = nullptr;
        }
    private:
        T*          m_Ptr;
        RefCount_t* m_RefCount;
        Allocator   m_Allocator;
    };

    template<typename T, typename Allocator = DefaultAllocator<T>, typename... Args>
    Scoped<T, Allocator> make_scoped(Args&&... args) {
        using AllocatorTraits = std::allocator_traits<Allocator>;
        Allocator alloc;
        auto*     ptr = AllocatorTraits::allocate(alloc, 1);
        AllocatorTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        return Scoped<T, Allocator>(ptr, alloc);
    }

    template<typename T, typename Allocator = DefaultAllocator<T>, typename... Args>
    Scoped<T, Allocator> make_scoped_with_allocator(Allocator alloc, Args&&... args) {
        using AllocatorTraits = std::allocator_traits<Allocator>;
        auto* ptr             = AllocatorTraits::allocate(alloc, 1);
        AllocatorTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        return Scoped<T, Allocator>(ptr, alloc);
    }

    template<typename T, typename Allocator = DefaultAllocator<T>, typename... Args>
    Ref<T, Allocator> make_ref(Args&&... args) {
        using AllocatorTraits = std::allocator_traits<Allocator>;
        Allocator alloc;
        auto*     ptr = AllocatorTraits::allocate(alloc, 1);
        AllocatorTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        return Ref<T, Allocator>(ptr, alloc);
    }

    template<typename T, typename Allocator = DefaultAllocator<T>, typename... Args>
    Ref<T, Allocator> make_ref_with_allocator(Allocator alloc, Args&&... args) {
        using AllocatorTraits = std::allocator_traits<Allocator>;
        auto* ptr             = AllocatorTraits::allocate(alloc, 1);
        AllocatorTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        return Ref<T, Allocator>(ptr, alloc);
    }
} // namespace Pulsar::GC
