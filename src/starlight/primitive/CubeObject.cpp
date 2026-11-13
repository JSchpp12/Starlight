#include "starlight/primitive/CubeObject.hpp"

#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"
#include "starlight/common/ConfigFile.hpp"
#include "starlight/common/materials/TextureMaterial.hpp"
#include "starlight/core/device/managers/Material.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/material/DefaultColorProvider.hpp"
#include "starlight/material/InstanceColorMaterial.hpp"
#include "starlight/material/MaterialProvider.hpp"
#include "starlight/primitive/MeshData.hpp"
#include "starlight/primitive/cube/MeshFactory.hpp"

#include <variant>

namespace star::primitive
{

static std::unique_ptr<star::StarMaterial> GetMaterial(const std::vector<cube::CubeDesc> &desc)
{
    // A texture-colored cube binds its texture through the material, so the first texture descriptor selects a
    // texture material for the object. Instances in an object share one material, so mixed descriptors cannot
    // each use their own.
    for (const auto &d : desc)
    {
        if (const auto *textureColor = std::get_if<cube::TextureColor>(&d.coloringApproach))
            return std::make_unique<star::TextureMaterial>(textureColor->getHandle());
    }

    std::vector<star::Color> colors;
    colors.reserve(desc.size());
    for (const auto &d : desc)
    {
        if (const auto *solidColor = std::get_if<cube::SolidColor>(&d.coloringApproach))
        {
            colors.push_back(solidColor->getColor());
        }
        else
        {
            // Texture-based coloring has no per-instance color.
            colors.push_back(star::Color{1.0f, 1.0f, 1.0f, 1.0f});
        }
    }

    return std::make_unique<star::material::InstanceColorMaterial>(
        std::make_unique<star::material::DefaultColorProvider>(std::move(colors)));
}

std::vector<std::unique_ptr<star::IMaterialProvider>> CubeObject::MakeMaterialProviders(
    const std::vector<cube::CubeDesc> &desc)
{
    std::vector<std::unique_ptr<star::IMaterialProvider>> providers;
    providers.push_back(std::make_unique<star::MaterialProvider>(GetMaterial(desc)));
    return providers;
}

CubeObject::CubeObject(std::vector<cube::CubeDesc> desc,
                       std::vector<std::unique_ptr<star::IMaterialProvider>> materialProviders,
                       const ShaderResolver &shaderResolver, OptionalRenderingOverrides renderOverrides)
    : StarObject(std::move(materialProviders)), m_desc(std::move(desc)), m_renderOverrides(std::move(renderOverrides))
{
    m_vertexShaderHandle = shaderResolver.resolve(Shader_Stage::vertex);
    m_fragmentShaderHandle = shaderResolver.resolve(Shader_Stage::fragment);

    for (size_t i{0}; i < m_desc.size(); i++)
    {
        createInstance();
    }
}

std::vector<StarMesh> CubeObject::loadMeshes(core::device::DeviceContext &context)
{
    const auto graphicsIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)

            ->getParentQueueFamilyIndex();

    // A line-list topology override is the sole signal for the wireframe mesh; per-cube desc data does not apply to it.
    const bool isWire =
        m_renderOverrides.topology.has_value() && *m_renderOverrides.topology == vk::PrimitiveTopology::eLineList;
    MeshData meshData = isWire ? cube::BuildCubeWireMesh() : cube::BuildCubeMesh();

    auto vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::VertInfo<>>(graphicsIndex, meshData.vertices));
    auto indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::IndicesInfo>(graphicsIndex, meshData.indices));

    return {StarMesh(vertBuffer, indBuffer, meshData.vertices, meshData.indices, m_materialHandles.front(), false)};
}

PipelineProvider CubeObject::getPipelineProvider(vk::PipelineLayout pipelineLayout)
{
    // Built here rather than delegating to StarObject::getPipelineProvider so the
    // object's render overrides can ride along. When nothing is overridden this
    // produces the same provider StarObject would have built.
    PipelineProvider::Builder builder = PipelineProvider::Builder()
                                            .setShaders({m_vertexShaderHandle, m_fragmentShaderHandle})
                                            .setPipelineLayout(pipelineLayout);

    GraphicsOverrides overrides{};
    overrides.topology = m_renderOverrides.topology;
    overrides.cullMode = m_renderOverrides.cullMode;
    overrides.depthStencil = m_renderOverrides.depthStencil;

    return builder.setGraphicsOverrides(std::move(overrides)).build();
}

} // namespace star::primitive