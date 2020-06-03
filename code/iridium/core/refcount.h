#pragma once

#include "atomic.h"

namespace Iridium
{
    class RefCounted : public Base
    {
    public:
        IR_FORCEINLINE constexpr RefCounted() noexcept = default;
        IR_FORCEINLINE ~RefCounted() override = default;

        IR_FORCEINLINE RefCounted(const RefCounted&) noexcept
        {
            // Don't modify ref_count_
        }

        IR_FORCEINLINE RefCounted& operator=(const RefCounted&) noexcept
        {
            // Don't modify ref_count_
            return *this;
        }

        IR_FORCEINLINE u32 AddRef() noexcept
        {
            return ++ref_count_;
        }

        IR_FORCEINLINE u32 Release()
        {
            u32 const refs = --ref_count_;

            if (refs == 0)
            {
                delete this;
            }

            return refs;
        }

        VIRTUAL_META_DECLARE;

    private:
        u32 ref_count_ {1};
    };

    class AtomicRefCounted : public Base
    {
    public:
        IR_FORCEINLINE constexpr AtomicRefCounted() noexcept = default;
        IR_FORCEINLINE ~AtomicRefCounted() override = default;

        IR_FORCEINLINE AtomicRefCounted(const AtomicRefCounted&) noexcept
        {
            // Don't modify ref_count_
        }

        IR_FORCEINLINE AtomicRefCounted& operator=(const AtomicRefCounted&) noexcept
        {
            // Avoid modifying ref_count_
            return *this;
        }

        IR_FORCEINLINE u32 AddRef() noexcept
        {
            return ref_count_.fetch_add(1, std::memory_order_relaxed);
        }

        IR_FORCEINLINE u32 Release()
        {
            u32 const refs = ref_count_.fetch_sub(1, std::memory_order_relaxed) - 1;

            if (refs == 0)
            {
                delete this;
            }

            return refs;
        }

        VIRTUAL_META_DECLARE;

    private:
        Atomic<u32> ref_count_ {1};
    };

    template <typename T>
    class Rc final
    {
    public:
        IR_FORCEINLINE constexpr Rc() noexcept = default;

        IR_FORCEINLINE constexpr Rc(std::nullptr_t) noexcept
            : ptr_(nullptr)
        {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE explicit Rc(U* ptr) noexcept
            : ptr_(ptr)
        {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE Rc(Ptr<U> ptr) noexcept
            : ptr_(ptr.release())
        {}

        IR_FORCEINLINE Rc(Rc&& other) noexcept
            : ptr_(other.release())
        {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE Rc(Rc<U>&& other) noexcept
            : ptr_(other.release())
        {}

        IR_FORCEINLINE Rc(const Rc& other) noexcept
            : ptr_(other.get())
        {
            if (ptr_)
                ptr_->AddRef();
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE Rc(const Rc<U>& other) noexcept
            : ptr_(other.get())
        {
            if (ptr_)
                ptr_->AddRef();
        }

        IR_FORCEINLINE ~Rc()
        {
            if (ptr_)
                ptr_->Release();
        }

        IR_FORCEINLINE Rc& operator=(Rc&& other)
        {
            Rc(std::move(other)).swap(*this);

            return *this;
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE Rc& operator=(Rc<U>&& other)
        {
            Rc(std::move(other)).swap(*this);

            return *this;
        }

        IR_FORCEINLINE Rc& operator=(const Rc& other)
        {
            Rc(other).swap(*this);

            return *this;
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE Rc& operator=(const Rc<U>& other)
        {
            Rc(other).swap(*this);

            return *this;
        }

        IR_FORCEINLINE void reset() noexcept
        {
            Rc().swap(*this);
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        IR_FORCEINLINE void reset(U* ptr)
        {
            Rc(ptr).swap(*this);
        }

        IR_FORCEINLINE T& operator*() const noexcept
        {
            return *ptr_;
        }

        IR_FORCEINLINE T* operator->() const noexcept
        {
            return ptr_;
        }

        IR_FORCEINLINE T* get() const noexcept
        {
            return ptr_;
        }

        IR_FORCEINLINE [[nodiscard]] T* release() noexcept
        {
            return std::exchange(ptr_, nullptr);
        }

        IR_FORCEINLINE void swap(Rc& other) noexcept
        {
            std::swap(ptr_, other.ptr_);
        }

        IR_FORCEINLINE explicit operator bool() const noexcept
        {
            return ptr_ != nullptr;
        }

        template <typename U>
        IR_FORCEINLINE bool operator==(const Rc<U>& other) const noexcept
        {
            return ptr_ == other.ptr_;
        }

        template <typename U>
        IR_FORCEINLINE bool operator!=(const Rc<U>& other) const noexcept
        {
            return ptr_ != other.ptr_;
        }

        IR_FORCEINLINE bool operator==(std::nullptr_t other) const noexcept
        {
            return ptr_ == other;
        }

        IR_FORCEINLINE bool operator!=(std::nullptr_t other) const noexcept
        {
            return ptr_ != other;
        }

    private:
        T* ptr_ {nullptr};
    };

    template <typename T>
    class StaticRc
    {
    public:
        template <typename... Args>
        inline StaticRc(Args&&... args)
            : value_(std::forward<Args>(args)...)
            , ref_(AddRc(&value_))
        {}

        IR_FORCEINLINE operator const Rc<T> &() noexcept
        {
            return ref_;
        }

    private:
        T value_;
        Rc<T> ref_;
    };

    template <typename T, typename... Args>
    [[nodiscard]] IR_FORCEINLINE std::enable_if_t<!std::is_array_v<T>, Rc<T>> MakeRc(Args&&... args)
    {
        return Rc<T>(new T(std::forward<Args>(args)...));
    }

    template <typename T>
    [[nodiscard]] IR_FORCEINLINE std::enable_if_t<!std::is_array_v<T>, Rc<T>> AddRc(T* ptr) noexcept
    {
        if (ptr)
            ptr->AddRef();

        return Rc<T>(ptr);
    }
} // namespace Iridium
