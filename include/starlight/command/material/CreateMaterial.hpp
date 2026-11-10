#pragma once

#include "MaterialTransferWait.hpp"

#include <starlight/virtual/StarMaterial.hpp>

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace star::command::material
{
namespace create_material
{
inline constexpr const char *GetTypeName()
{
    return "ScMatC";
}
} // namespace create_material

/// @brief Create a material and return its MaterialManager handle.
struct CreateMaterial : public star::common::IServiceCommandWithReply<Handle>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return create_material::GetTypeName();
    }

    explicit CreateMaterial(std::unique_ptr<star::StarMaterial> material, std::vector<MaterialTransferWait> waits = {})
        : material(std::move(material)), waits(std::move(waits))
    {
    }

    std::unique_ptr<star::StarMaterial> material;
    std::vector<MaterialTransferWait> waits;
};
} // namespace star::command::material
