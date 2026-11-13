#include "TextureMaterial.hpp"

#include "FileHelpers.hpp"
#include "ManagerRenderResource.hpp"
#include "StarShaderInfo.hpp"
#include "TransferRequest_CompressedTextureFile.hpp"
#include "TransferRequest_TextureFile.hpp"
#include "core/device/managers/Image.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/helper/queue/QueueHelpers.hpp"
#include "starlight/command/transfer/SubmitTextureTransfer.hpp"
#include "starlight/core/transfer/RegisterTransferWait.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <cassert>
#include <memory>
#include <utility>

star::TextureMaterial::TextureMaterial(std::string texturePath,
                                       std::unique_ptr<SharedCompressedTexture> preTranscodedTexture)
    : m_texturePath(std::move(texturePath)), m_preTranscodedTexture(std::move(preTranscodedTexture))
{
    if (!file_helpers::FileExists(m_texturePath))
    {
        STAR_THROW("Provided texture path for material does not exist: " + m_texturePath);
    }
}

star::TextureMaterial::~TextureMaterial() = default;

star::TextureMaterial::TextureMaterial(Handle preloadedTexture) : m_textureHandle(std::move(preloadedTexture))
{
    if (!m_textureHandle.isInitialized())
        STAR_THROW("TextureMaterial requires an initialized texture handle");
}

star::TextureMaterial::TextureMaterial(std::string texturePath, glm::vec4 surfaceColor, glm::vec4 highlightColor,
                                       glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, int shiny)
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

void star::TextureMaterial::preloadTexture(core::device::DeviceContext &context)
{
    if (m_textureHandle.isInitialized())
        return;

    const auto graphicsIndex =
        core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                            star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    auto deviceProperties = context.getDevice().getPhysicalDevice().getProperties();

    auto texture = std::unique_ptr<TransferRequest::Texture>();
    if (TransferRequest::CompressedTextureFile::IsFileCompressedTexture(m_texturePath))
    {
        std::unique_ptr<SharedCompressedTexture> compressed = std::move(m_preTranscodedTexture);
        if (!compressed)
        {
            compressed = std::make_unique<SharedCompressedTexture>(
                SharedCompressedTexture::Builder()
                    .setPath(m_texturePath)
                    .setAttemptGPUCompressionScheme(context.getDevice().getPhysicalDevice())
                    .build());
        }

        texture = std::make_unique<TransferRequest::CompressedTextureFile>(
            std::move(graphicsIndex), std::move(deviceProperties), std::move(compressed));
    }
    else
    {
        texture = std::make_unique<TransferRequest::TextureFile>(std::move(graphicsIndex), std::move(deviceProperties),
                                                                 m_texturePath);
    }

    assert(texture && "Texture MUST be created");

    star::command::transfer::SubmitTextureTransfer transfer{std::move(texture)};
    context.getCmdBus().submit(transfer);
    const auto submission = transfer.getReply().get();

    m_textureHandle = submission.image;
    m_textureTransfer = submission.semaphore;
}

void star::TextureMaterial::prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight,
                                       star::StarShaderInfo::Builder frameBuilder, star::Handle commandBuffer)
{
    preloadTexture(context);
    if (!m_resourceHandle.isInitialized())
        m_resourceHandle = m_textureHandle;

    if (m_textureHandle.isInitialized())
        registerTextureTransferWait(context, commandBuffer, m_textureHandle, m_textureTransfer,
                                    m_textureTransferStages);

    StarMaterial::prepRender(context, numFramesInFlight, frameBuilder, commandBuffer);
}

void star::TextureMaterial::registerTextureTransferWait(core::device::DeviceContext &context,
                                                        star::Handle commandBuffer, star::Handle textureHandle,
                                                        const star::StarSemaphore &transferSemaphore,
                                                        vk::PipelineStageFlags waitStages)
{
    // A preloaded texture has no transfer of its own, so there is nothing to wait on.
    if (!textureHandle.isInitialized() || transferSemaphore.vkSemaphore == VK_NULL_HANDLE)
        return;

    star::core::transfer::RegisterTransferWait(context, commandBuffer, textureHandle, transferSemaphore.vkSemaphore,
                                               transferSemaphore.signalValue, waitStages);
}

std::unique_ptr<star::StarShaderInfo> star::TextureMaterial::buildShaderInfo(core::device::DeviceContext &context,
                                                                             uint8_t numFramesInFlight,
                                                                             StarShaderInfo::Builder builder)
{
    assert(m_textureHandle.isInitialized() && "Material needs to be prepared before it can be used");

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        builder.startOnFrameIndex(i);
        builder.startSet();
        builder.add(StarShaderInfo::TextureInfo{m_textureHandle, vk::ImageLayout::eShaderReadOnlyOptimal});
    }

    return builder.build();
}

std::vector<std::pair<vk::DescriptorType, const int>> star::TextureMaterial::getDescriptorRequests(
    int numFramesInFlight) const
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 2 * numFramesInFlight)};
}