#pragma once

#include <starlight/setup/RenderResourceSetup.hpp>

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>

#include <string_view>
#include <utility>
#include <vector>

namespace star::command_order
{
namespace add_render_resource_setup
{
inline constexpr const char *GetTypeName() noexcept
{
    return "coAddRRS";
}
} // namespace add_render_resource_setup

/// @brief Hands already-prepared one-shot work to the owning command buffer's CompleteRequest.
struct AddRenderResourceSetup : public star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName() noexcept
    {
        return add_render_resource_setup::GetTypeName();
    }

    AddRenderResourceSetup(
        Handle commandBuffer,
        std::vector<setup::RenderResourceSetup::ResourcePrepDefinition> definitions)
        : commandBuffer(std::move(commandBuffer)), definitions(std::move(definitions))
    {
    }

    /// @brief Command buffer whose CompleteRequest receives the work.
    Handle commandBuffer;
    std::vector<setup::RenderResourceSetup::ResourcePrepDefinition> definitions;
};
} // namespace star::command_order
