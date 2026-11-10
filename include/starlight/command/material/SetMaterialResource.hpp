#pragma once

#include <starlight/setup/RenderResourceSetup.hpp>
#include <starlight/wrappers/graphics/StarSemaphore.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>

#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace star::command::material
{
namespace set_material_resource
{
inline constexpr const char *GetTypeName()
{
    return "ScMatR";
}
} // namespace set_material_resource

/// @brief Retarget a material's primary resource and carry the one-shot work that must precede sampling it.
struct SetMaterialResource : public star::common::IServiceCommand
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return set_material_resource::GetTypeName();
    }

    SetMaterialResource(Handle material, Handle recordingCommandBuffer, size_t setIndex, size_t bindingIndex,
                        StarShaderInfo::ShaderInfo newHandle, std::vector<setup::TransferWait> waits,
                        star::setup::RenderResourceSetup::PrepRequest prep,
                        std::optional<size_t> frameInFlightIndex = std::nullopt)
        : material(std::move(material)), recordingCommandBuffer(std::move(recordingCommandBuffer)), setIndex(setIndex),
          bindingIndex(bindingIndex), newHandle(std::move(newHandle)), frameInFlightIndex(frameInFlightIndex),
          waits(std::move(waits)), prep(std::move(prep))
    {
    }

    Handle material;
    /// @brief Command buffer of the render phase that owns the resource being swapped.
    /// @details The service registers the transfer waits with this buffer's one-time wait info.
    Handle recordingCommandBuffer;
    /// @brief Which binding of the material's own StarShaderInfo the new resource replaces.
    size_t setIndex;
    size_t bindingIndex;
    /// @brief Resolved description of the new resource (buffer or texture).
    StarShaderInfo::ShaderInfo newHandle;
    /// @brief Restrict the replacement to one frame in flight; empty replaces it in every frame.
    std::optional<size_t> frameInFlightIndex;
    std::vector<setup::TransferWait> waits;
    /// @brief One-shot work for the new resource. CommandOrderService is bypassed; the service prepares it directly.
    star::setup::RenderResourceSetup::PrepRequest prep;
};
} // namespace star::command::material
