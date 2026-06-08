#pragma once

#include "starlight/object/StarObject.hpp"
#include "starlight/primitive/SquareDesc.hpp"
#include <Enums.hpp>
#include <StarMesh.hpp>
#include <StarShader.hpp>
#include <device/DeviceContext.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace star::primitive
{

class SquareObject : public StarObject
{
  public:
    explicit SquareObject(SquareDesc desc = {});

    std::unordered_map<Shader_Stage, StarShader> getShaders() override;

  protected:
    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;

  private:
    SquareDesc m_desc;
};

} // namespace star::primitive