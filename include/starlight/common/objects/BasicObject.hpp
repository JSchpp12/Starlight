#pragma once

#include "StarObject.hpp"

#include <tiny_obj_loader.h>

#include <memory>
#include <string>
#include <vector>

namespace star
{
/// <summary>
/// Basic object for use with rendering. This object is loaded from an .obj file
/// and is attached to a simple shader with textures and a graphics pipeline for
/// those shader types.
/// </summary>
class BasicObject : public StarObject
{
  public:
    BasicObject(std::string objFilePath)
        : StarObject(LoadMaterials(objFilePath)), m_objFilePath(std::move(objFilePath)) 
    {
    }

    virtual ~BasicObject() = default;

    virtual std::unordered_map<star::Shader_Stage, StarShader> getShaders() override;

  protected:
    std::string m_objFilePath = "";

    Handle primaryVertBuffer, primaryIndbuffer;

    std::string objectFilePath;

    std::vector<std::unique_ptr<StarMesh>> loadMeshes(core::device::DeviceContext &context) override;

    std::vector<std::shared_ptr<StarMaterial>> LoadMaterials(const std::string &filePath);

    void getTypeOfMaterials(bool &isTextureMaterial, bool &isBumpMaterial) const; 
};
} // namespace star