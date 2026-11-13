#pragma once

#include "starlight/structs/MaterialSwapCommitInfo.hpp"

#include <functional>
#include <span>

namespace star::event::material_swap
{
/// @brief Transient view over the commits collected for one dispatch.
/// @details Holds a span into MaterialService's reusable buffer. Non-copyable and non-movable; iterate via forEach.
/// The type name marks the contents as transient: do not retain a reference past the synchronous emit.
class SwapResultView
{
  public:
    explicit SwapResultView(std::span<const MaterialSwapCommitInfo> results) noexcept : m_results(results)
    {
    }

    SwapResultView(const SwapResultView &) = delete;
    SwapResultView &operator=(const SwapResultView &) = delete;
    SwapResultView(SwapResultView &&) = delete;
    SwapResultView &operator=(SwapResultView &&) = delete;

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_results.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_results.empty();
    }

    template <typename F> void forEach(F &&fn) const
    {
        for (const auto &result : m_results)
            std::invoke(fn, result);
    }

  private:
    std::span<const MaterialSwapCommitInfo> m_results;
};
} // namespace star::event::material_swap
