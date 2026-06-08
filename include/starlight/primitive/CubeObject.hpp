#pragma once

#include "starlight/object/StarObject.hpp"
#include "starlight/primitive/CubeDesc.hpp"
#include <Enums.hpp>
#include <StarMesh.hpp>
#include <StarShader.hpp>
#include <device/DeviceContext.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace star::primitive
{

class CubeObject : public StarObject
{
  public:
    explicit CubeObject(std::vector<CubeDesc> desc = {});

    std::unordered_map<Shader_Stage, StarShader> getShaders() override;

  protected:
    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;

  private:
    std::vector<CubeDesc> m_desc;
};

} // namespace star::primitive