#include "starlight/primitive/CubeObject.hpp"

#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"
#include "starlight/common/ConfigFile.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/material/DefaultColorProvider.hpp"
#include "starlight/material/InstanceColorMaterial.hpp"
#include "starlight/primitive/MeshData.hpp"
#include "starlight/primitive/cube/MeshFactory.hpp"

#include <variant>

namespace star::primitive
{

static std::shared_ptr<star::material::InstanceColorMaterial> GetMaterial(const std::vector<cube::CubeDesc> &desc)
{
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
            // Texture-based coloring has no per-instance color yet.
            colors.push_back(star::Color{1.0f, 1.0f, 1.0f, 1.0f});
        }
    }

    return std::make_shared<star::material::InstanceColorMaterial>(
        std::make_unique<star::material::DefaultColorProvider>(std::move(colors)));
}

CubeObject::CubeObject(std::vector<cube::CubeDesc> desc, const ShaderResolver &shaderResolver,
                       OptionalRenderingOverrides renderOverrides)
    : StarObject({GetMaterial(desc)}), m_desc(std::move(desc)), m_renderOverrides(std::move(renderOverrides))
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

    const bool isWire =
        m_renderOverrides.topology.has_value() && *m_renderOverrides.topology == vk::PrimitiveTopology::eLineList;
    MeshData meshData = isWire ? cube::BuildCubeWireMesh() : cube::BuildCubeMesh();

    auto vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::VertInfo<>>(graphicsIndex, meshData.vertices));
    auto indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::IndicesInfo>(graphicsIndex, meshData.indices));

    return {StarMesh(vertBuffer, indBuffer, meshData.vertices, meshData.indices, m_meshMaterials.front(), false)};
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