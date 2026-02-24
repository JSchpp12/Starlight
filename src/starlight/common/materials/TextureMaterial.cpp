#include "TextureMaterial.hpp"

#include "FileHelpers.hpp"
#include "ManagerRenderResource.hpp"
#include "StarShaderInfo.hpp"
#include "TransferRequest_CompressedTextureFile.hpp"
#include "TransferRequest_TextureFile.hpp"
#include "core/helper/queue/QueueHelpers.hpp"

#include <cassert>

star::TextureMaterial::TextureMaterial(std::string texturePath, const glm::vec4 &surfaceColor,
                                       const glm::vec4 &highlightColor, const glm::vec4 &ambient,
                                       const glm::vec4 &diffuse, const glm::vec4 &specular, const int &shiny)
    : StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
      m_texturePath(std::move(texturePath))
{
    if (!file_helpers::FileExists(m_texturePath))
    {
        STAR_THROW("Provided texture path for material does not exist: " + m_texturePath);
    }
}

void star::TextureMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const
{
    constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler,
                            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

star::TextureMaterial::TextureMaterial(std::string texturePath) : m_texturePath(std::move(texturePath))
{
    if (!file_helpers::FileExists(m_texturePath))
    {
        STAR_THROW("Provided texture path for material does not exist: " + m_texturePath);
    }
}

void star::TextureMaterial::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                       star::StarShaderInfo::Builder frameBuilder)
{
    if (!m_textureHandle.isInitialized())
    {
        const auto texSemaphore = context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest(false));
        const auto graphicsIndex =
            core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                star::Queue_Type::Tgraphics)
                ->getParentQueueFamilyIndex();
        const auto deviceProperties = context.getDevice().getPhysicalDevice().getProperties();

        auto texture = std::unique_ptr<TransferRequest::Texture>();
        if (TransferRequest::CompressedTextureFile::IsFileCompressedTexture(m_texturePath))
        {
            texture = std::make_unique<TransferRequest::CompressedTextureFile>(
                std::move(graphicsIndex), std::move(deviceProperties),
                std::make_unique<SharedCompressedTexture>(m_texturePath));
        }
        else
        {
            texture = std::make_unique<TransferRequest::TextureFile>(std::move(graphicsIndex),
                                                                     std::move(deviceProperties), m_texturePath);
        }

        assert(texture && "Texture MUST be created");

        m_textureHandle = star::ManagerRenderResource::addRequest(
            context.getDeviceID(), context.getSemaphoreManager().get(texSemaphore)->semaphore, std::move(texture));
    }

    StarMaterial::prepRender(context, numFramesInFlight, frameBuilder);
}

std::unique_ptr<star::StarShaderInfo> star::TextureMaterial::buildShaderInfo(core::device::DeviceContext &context,
                                                                             const uint8_t &numFramesInFlight,
                                                                             StarShaderInfo::Builder builder)
{
    assert(m_textureHandle.isInitialized() && "Material needs to be prepared before it can be used");

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        builder.startOnFrameIndex(i);
        builder.startSet();
        builder.add(m_textureHandle, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    return builder.build();
}

std::vector<std::pair<vk::DescriptorType, const int>> star::TextureMaterial::getDescriptorRequests(
    const int &numFramesInFlight) const
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, numFramesInFlight)};
}