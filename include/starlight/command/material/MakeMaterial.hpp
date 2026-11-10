#pragma once

#include <starlight/setup/RenderResourceSetup.hpp>
#include <starlight/virtual/StarMaterial.hpp>

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace star::command::material
{
namespace make_material
{
inline constexpr const char *GetTypeName()
{
    return "ScMatM";
}
} // namespace make_material

/// @brief Create a material at prepRender and apply its waits/prep directly to the owning pass.
/// @details The service inserts the waits into the command buffer's one-time wait list, prepares the prep into
/// a definition owned by that pass's CompleteRequest, and submits only the material to the manager.
struct MakeMaterial : public star::common::IServiceCommandWithReply<Handle>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return make_material::GetTypeName();
    }

    MakeMaterial(std::unique_ptr<StarMaterial> material, std::vector<setup::TransferWait> waits,
                 setup::RenderResourceSetup::PrepRequest prep, Handle commandBuffer)
        : material(std::move(material)), waits(std::move(waits)), prep(std::move(prep)),
          commandBuffer(std::move(commandBuffer))
    {
    }

    std::unique_ptr<StarMaterial> material;
    std::vector<setup::TransferWait> waits;
    setup::RenderResourceSetup::PrepRequest prep;
    /// @brief Command buffer of the pass that records this material.
    Handle commandBuffer;
};
} // namespace star::command::material
