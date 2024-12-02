#pragma once

// TODO: Maybe look into using private inheritance to make this more efficient (std::allocator)

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

    template<typename T, typename Allocator = DefaultAllocator<T>, typename... Args>
    Scoped<T, Allocator> make_scoped(Args&&... args) {
        using AllocatorTraits = std::allocator_traits<Allocator>;
        Allocator alloc;
        auto*     ptr = AllocatorTraits::allocate(alloc, 1);
        AllocatorTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        return Scoped<T, Allocator>(ptr, alloc);
    }
} // namespace Pulsar::GC
