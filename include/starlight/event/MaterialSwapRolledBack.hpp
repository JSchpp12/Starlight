#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IEvent.hpp>

#include <string_view>
#include <utility>

namespace star::event
{
namespace material_swap_rolled_back
{
inline constexpr const char *GetUniqueTypeName()
{
    return "EvtMSRB";
}
} // namespace material_swap_rolled_back

/// @brief A material swap failed its record-time readiness checks and was reverted to the previous resource.
class MaterialSwapRolledBack : public common::IEvent
{
  public:
    MaterialSwapRolledBack(Handle material, Handle previousResource);

    static constexpr std::string_view GetUniqueTypeName()
    {
        return material_swap_rolled_back::GetUniqueTypeName();
    }

    [[nodiscard]] const Handle &getMaterial() const noexcept
    {
        return m_material;
    }

    [[nodiscard]] const Handle &getPreviousResource() const noexcept
    {
        return m_previousResource;
    }

  private:
    Handle m_material;
    Handle m_previousResource;
};
} // namespace star::event
