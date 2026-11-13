#pragma once

#include "starlight/ShaderResolver.hpp"
#include "starlight/object/StarObject.hpp"

#include <tiny_obj_loader.h>

#include <memory>
#include <optional>
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
    BasicObject(std::string objFilePath, std::vector<std::unique_ptr<IMaterialProvider>> materialProviders,
                ShaderResolver &shaderResolver);
    BasicObject(std::string objFilePath, const std::filesystem::path &materialDir,
                std::vector<std::unique_ptr<IMaterialProvider>> materialProviders, ShaderResolver &shaderResolver);
    virtual ~BasicObject() = default;

    /// @brief Build the deferred material recipes for this object.
    static std::vector<std::unique_ptr<IMaterialProvider>> MakeMaterialProviders(
        core::device::DeviceContext &context, const std::string &objFilePath,
        std::optional<std::filesystem::path> materialDir = std::nullopt);

    static ShaderResolver PrepareResolver(const std::string &objFilePath, core::CommandBus &bus);
    static ShaderResolver PrepareResolver(const std::string &objFilePath, const std::filesystem::path &materialDir,
                                          core::CommandBus &bus);

  protected:
    std::string m_objFilePath = "";

    Handle primaryVertBuffer, primaryIndbuffer;

    std::string objectFilePath;

    std::vector<StarMesh> loadMeshes(core::device::DeviceContext &context) override;
};
} // namespace star