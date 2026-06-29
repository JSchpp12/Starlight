#include "starlight/primitive/SquareObject.hpp"

#include "starlight/common/ConfigFile.hpp"
#include "starlight/primitive/MeshData.hpp"
#include "starlight/primitive/SquareMeshBuilder.hpp"

namespace star::primitive
{
SquareObject::SquareObject(SquareDesc desc, ShaderResolver &shaderResolver) : StarObject(), m_desc(std::move(desc))
{
    m_vertexShaderHandle = shaderResolver.resolve(Shader_Stage::vertex);
    m_fragmentShaderHandle = shaderResolver.resolve(Shader_Stage::fragment);
}

std::vector<StarMesh> SquareObject::loadMeshes(core::device::DeviceContext &context)
{
    MeshData meshData = BuildSquareMesh(m_desc);
    return {};
}

} // namespace star::primitive
