#pragma once

#include "starlight/ShaderResolver.hpp"
#include "starlight/object/StarObject.hpp"
#include "starlight/primitive/SquareDesc.hpp"
#include <Enums.hpp>
#include <StarMesh.hpp>
#include <device/DeviceContext.hpp>

#include <memory>
#include <vector>

namespace star::primitive
{

class SquareObject : public StarObject
{
  public:
    explicit SquareObject(SquareDesc desc, ShaderResolver &shaderResolver);

  protected:
    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;

  private:
    SquareDesc m_desc;
};

} // namespace star::primitive