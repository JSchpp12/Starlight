#pragma once

#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace star::core
{
/// Small-buffer type-erased callable with a hard inline-storage cap.
///
/// `Sig` is a function type (e.g. `void(int, RenderPhase&)`); `N` is the inline payload
/// capacity in bytes. SboCallable is move-only and fixed-size; stored by value it is an inline
/// sub-object of its owner, preserving contiguous storage.
template <typename Sig, size_t N> class SboCallable;

template <typename R, typename... Args, size_t N> class SboCallable<R(Args...), N>
{
    static_assert(N > 0, "SboCallable inline storage size N must be greater than 0");

  public:
    /// Internal exec trampoline type: the user-facing Sig with a void* payload prepended.
    using ExecFn = R (*)(void *, Args...);
    using DestroyFn = void (*)(void *);
    using MoveFn = void (*)(void *dest, void *src);

    SboCallable() = default;

    ~SboCallable()
    {
        if (m_destroy)
            m_destroy(m_buf);
    }

    SboCallable(SboCallable &&other) noexcept
    {
        // Use the SOURCE move trampoline to relocate the payload (it knows the concrete T),
        // then adopt the trampolines and null the source -- exactly like Task's move ctor.
        if (other.m_move)
        {
            other.m_move(m_buf, other.m_buf);
            m_exec = other.m_exec;
            m_destroy = other.m_destroy;
            m_move = other.m_move;
            other.m_exec = nullptr;
            other.m_destroy = nullptr;
            other.m_move = nullptr;
        }
    }

    SboCallable &operator=(SboCallable &&other) noexcept
    {
        if (this != &other)
        {
            if (m_destroy)
                m_destroy(m_buf);

            // Adopt the source trampolines first, then relocate the payload with them, then
            // null the source. (m_destroy above already released our old payload.)
            m_exec = other.m_exec;
            m_destroy = other.m_destroy;
            m_move = other.m_move;

            if (m_move)
                m_move(m_buf, other.m_buf);

            other.m_exec = nullptr;
            other.m_destroy = nullptr;
            other.m_move = nullptr;
        }
        return *this;
    }

    SboCallable(const SboCallable &) = delete;
    SboCallable &operator=(const SboCallable &) = delete;

    /// Construct a concrete functor `T` inline and bind its trampolines.
    /// Hard compile-time cap (invariant I3): the payload must fit inline and must not be
    /// over-aligned beyond std::max_align_t (invariant I4). No heap fallback.
    template <typename T> static SboCallable make(T &&payload)
    {
        using Payload = std::decay_t<T>;

        static_assert(sizeof(Payload) <= N, "Payload exceeds SboCallable inline storage");
        static_assert(alignof(Payload) <= alignof(std::max_align_t), "Payload alignment too strict for inline storage");

        SboCallable s;
        new (static_cast<void *>(s.m_buf)) Payload(std::forward<T>(payload));
        s.m_exec = &trampolineExec<Payload>;
        s.m_destroy = &trampolineDestroy<Payload>;
        s.m_move = &trampolineMove<Payload>;
        return s;
    }

    /// Invoke the stored payload with the Sig arguments. Mirrors std::function's const
    /// operator() invoking a (potentially mutating) target: the inline buffer is `mutable` so a
    /// const SboCallable can still dispatch a non-const operator() on its payload.
    R exec(Args... args) const
    {
        assert(m_exec && "SboCallable::exec called on an empty callable");
        return m_exec(m_buf, std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept
    {
        return m_exec != nullptr;
    }

  private:
    template <typename T> static R trampolineExec(void *buf, Args... args)
    {
        return static_cast<T *>(buf)->operator()(std::forward<Args>(args)...);
    }

    template <typename T> static void trampolineDestroy(void *buf) noexcept
    {
        static_cast<T *>(buf)->~T();
    }

    template <typename T> static void trampolineMove(void *dest, void *src)
    {
        T *srcPayload = static_cast<T *>(src);
        new (dest) T(std::move(*srcPayload));
        srcPayload->~T();
    }

    // `mutable` so exec() (const) can dispatch a mutating payload, matching std::function.
    alignas(std::max_align_t) mutable std::byte m_buf[N]{};

    ExecFn m_exec = nullptr;
    DestroyFn m_destroy = nullptr;
    MoveFn m_move = nullptr;
};
} // namespace star::core
