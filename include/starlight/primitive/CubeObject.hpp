#pragma once

#include "starlight/ShaderResolver.hpp"
#include "starlight/object/StarObject.hpp"
#include "starlight/primitive/cube/CubeDesc.hpp"
#include <StarMesh.hpp>
#include <device/DeviceContext.hpp>

#include <optional>
#include <vector>

namespace star::primitive
{

class CubeObject : public StarObject
{
  public:
    /// @brief Lean render-state override set for a cube object's pipeline to override engine defaults.
    struct OptionalRenderingOverrides
    {
        std::optional<vk::PrimitiveTopology> topology{std::nullopt};
        std::optional<vk::CullModeFlags> cullMode{std::nullopt};
        std::optional<vk::PipelineDepthStencilStateCreateInfo> depthStencil{std::nullopt};
    };

    /// @brief Default (no-op) render override set.
    /// @note This is a function rather than a plain `OptionalRenderingOverrides{}` default argument because GCC rejects
    /// a nested class's default member initializers when they are needed by a default argument of the enclosing class
    static OptionalRenderingOverrides DefaultRenderOverrides()
    {
        return {};
    }

    /// @brief Create a cube object.
    /// @param renderOverrides Optional render-state changes for this object's pipeline (topology, cull mode,
    /// depth/stencil).
    CubeObject(std::vector<cube::CubeDesc> desc, std::vector<Handle> materialHandles,
               const ShaderResolver &shaderResolver,
               OptionalRenderingOverrides renderOverrides = DefaultRenderOverrides());

    /// @brief Build and submit the material for these cube descriptors, returning its manager handle.
    static std::vector<Handle> MakeMaterialHandles(core::device::DeviceContext &context,
                                                   const std::vector<cube::CubeDesc> &desc);

  protected:
    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;

    PipelineProvider getPipelineProvider(vk::PipelineLayout pipelineLayout) override;

  private:
    std::vector<cube::CubeDesc> m_desc;
    OptionalRenderingOverrides m_renderOverrides;
};

} // namespace star::primitive