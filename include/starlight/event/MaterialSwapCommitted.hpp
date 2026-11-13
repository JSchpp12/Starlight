#pragma once

#include "starlight/event/material_swap/SwapResultView.hpp"
#include "starlight/structs/MaterialSwapCommitInfo.hpp"

#include <star_common/IEvent.hpp>

#include <span>
#include <string_view>

namespace star::event
{
namespace material_swap_committed
{
inline constexpr const char *GetUniqueTypeName()
{
    return "EvtMSC";
}
} // namespace material_swap_committed

/// @brief Batched terminal success notification for material swaps.
/// @details The event is non-copyable/non-movable; EventBus dispatches it synchronously by const reference.
class MaterialSwapCommitted : public common::IEvent
{
  public:
    explicit MaterialSwapCommitted(std::span<const MaterialSwapCommitInfo> results);

    MaterialSwapCommitted(const MaterialSwapCommitted &) = delete;
    MaterialSwapCommitted &operator=(const MaterialSwapCommitted &) = delete;
    MaterialSwapCommitted(MaterialSwapCommitted &&) = delete;
    MaterialSwapCommitted &operator=(MaterialSwapCommitted &&) = delete;

    static constexpr std::string_view GetUniqueTypeName()
    {
        return material_swap_committed::GetUniqueTypeName();
    }

    [[nodiscard]] const material_swap::SwapResultView &getResults() const noexcept
    {
        return m_results;
    }

  private:
    material_swap::SwapResultView m_results;
};
} // namespace star::event
