#include "starlight/primitive/CubeObject.hpp"

#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"
#include "starlight/common/ConfigFile.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/material/DefaultColorProvider.hpp"
#include "starlight/material/InstanceColorMaterial.hpp"
#include "starlight/primitive/CubeMeshBuilder.hpp"
#include "starlight/primitive/MeshData.hpp"

namespace star::primitive
{

static std::shared_ptr<star::material::InstanceColorMaterial> GetMaterial(const std::vector<CubeDesc> &desc)
{
    std::vector<star::Color> colors;
    colors.reserve(desc.size());
    for (const auto &d : desc)
    {
        colors.push_back(d.color);
    }

    return std::make_shared<star::material::InstanceColorMaterial>(
        std::make_unique<star::material::DefaultColorProvider>(std::move(colors)));
}

CubeObject::CubeObject(std::vector<CubeDesc> desc) : StarObject({GetMaterial(desc)}), m_desc(std::move(desc))
{
    for (size_t i{0}; i < m_desc.size(); i++)
    {
        createInstance();
    }
}

std::unordered_map<Shader_Stage, StarShader> CubeObject::getShaders()
{
    std::unordered_map<Shader_Stage, StarShader> shaders;

    const std::filesystem::path sPath =
        std::filesystem::path{star::ConfigFile::getSetting(Config_Settings::mediadirectory)} / "shaders" / "debugCube";
    {
        const std::filesystem::path vert = sPath / "debugCube.vert";
        shaders.emplace(Shader_Stage::vertex, StarShader(vert.string(), Shader_Stage::vertex));
    }

    {
        const std::filesystem::path frag = sPath / "debugCube.frag";
        shaders.emplace(Shader_Stage::fragment, StarShader(frag.string(), Shader_Stage::fragment));
    }

    return shaders;
}

std::vector<StarMesh> CubeObject::loadMeshes(core::device::DeviceContext &context)
{
    const auto graphicsIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)

            ->getParentQueueFamilyIndex();
    MeshData meshData = BuildCubeMesh(m_desc, this->m_meshMaterials.front());

    auto semaphore = context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));
    auto indSemaphore = context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    auto vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(semaphore)->semaphore,
        std::make_unique<star::TransferRequest::VertInfo>(graphicsIndex, meshData.vertices));
    auto indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(indSemaphore)->semaphore,
        std::make_unique<TransferRequest::IndicesInfo>(graphicsIndex, meshData.indices));

    return {StarMesh(vertBuffer, indBuffer, meshData.vertices, meshData.indices, m_meshMaterials.front(), false)};
}

} // namespace star::primitive