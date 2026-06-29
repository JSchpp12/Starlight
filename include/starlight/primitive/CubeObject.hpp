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
    CubeObject(std::vector<CubeDesc> desc, ShaderResolver &shaderResolver);

  protected:
    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;

  private:
    std::vector<CubeDesc> m_desc;
};

} // namespace star::primitive