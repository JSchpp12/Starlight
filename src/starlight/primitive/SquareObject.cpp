#include "starlight/primitive/SquareObject.hpp"

#include "starlight/common/ConfigFile.hpp"
#include "starlight/primitive/MeshData.hpp"
#include "starlight/primitive/SquareMeshBuilder.hpp"

namespace star::primitive
{
SquareObject::SquareObject(SquareDesc desc) : StarObject(), m_desc(std::move(desc))
{
}

std::unordered_map<Shader_Stage, StarShader> SquareObject::getShaders()
{
    std::unordered_map<Shader_Stage, StarShader> shaders;

    shaders.emplace(Shader_Stage::vertex, StarShader(star::ConfigFile::getSetting(Config_Settings::mediadirectory) +
                                                         "/shaders/vertColor.vert",
                                                     Shader_Stage::vertex));

    shaders.emplace(Shader_Stage::fragment, StarShader(star::ConfigFile::getSetting(Config_Settings::mediadirectory) +
                                                           "/shaders/vertColor.frag",
                                                       Shader_Stage::fragment));

    return shaders;
}

std::vector<StarMesh> SquareObject::loadMeshes(core::device::DeviceContext &context)
{
    MeshData meshData = BuildSquareMesh(m_desc);
    return {};
}

} // namespace star::primitive
