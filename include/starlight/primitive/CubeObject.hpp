#pragma once

#include "starlight/ShaderResolver.hpp"
#include "starlight/object/StarObject.hpp"
#include "starlight/primitive/CubeDesc.hpp"
#include <Enums.hpp>
#include <StarMesh.hpp>
#include <device/DeviceContext.hpp>

#include <memory>
#include <vector>

namespace star::primitive
{

class CubeObject : public StarObject
{
  public:
    /// @brief Topology type used
    enum class RenderMode
    {
        triangles,
        lines
    };

    CubeObject(std::vector<CubeDesc> desc, ShaderResolver &shaderResolver,
               RenderMode renderMode = RenderMode::triangles);

  protected:
    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;

    PipelineProvider getPipelineProvider(vk::PipelineLayout pipelineLayout) override;

  private:
    std::vector<CubeDesc> m_desc;
    RenderMode m_renderMode{RenderMode::triangles};
};

} // namespace star::primitive