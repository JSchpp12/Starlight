#include "BumpMaterial.hpp"

#include "core/helper/queue/QueueHelpers.hpp"
#include "event/GetQueue.hpp"
#include "starlight/command/transfer/SubmitTextureTransfer.hpp"
#include <starlight/common/textures/TransferRequest_CompressedTextureFile.hpp>
#include <starlight/common/textures/TransferRequest_TextureFile.hpp>

void star::BumpMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const
{
    constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::BumpMaterial::prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight,
                                    star::StarShaderInfo::Builder frameBuilder, star::Handle commandBuffer)
{
    if (!m_bumpMap.isInitialized())
    {
        std::unique_ptr<star::TransferRequest::Texture> bumpMap;
        if (star::TransferRequest::CompressedTextureFile::IsFileCompressedTexture(m_bumpMapFilePath))
        {
            bumpMap = std::make_unique<TransferRequest::CompressedTextureFile>(
                star::core::helper::GetEngineDefaultQueue(
                    context.getEventBus(), context.getGraphicsManagers().queueManager, Queue_Type::Tgraphics)
                    ->getParentQueueFamilyIndex(),
                context.getDevice().getPhysicalDevice().getProperties(),
                std::make_unique<SharedCompressedTexture>(SharedCompressedTexture::Builder()
                                                              .setPath(m_bumpMapFilePath)
                                                              .setNoAttemptGPUCompression()
                                                              .build()));
        }
        else
        {
            bumpMap = std::make_unique<TransferRequest::TextureFile>(
                star::core::helper::GetEngineDefaultQueue(
                    context.getEventBus(), context.getGraphicsManagers().queueManager, Queue_Type::Tgraphics)
                    ->getParentQueueFamilyIndex(),
                context.getDevice().getPhysicalDevice().getProperties(), m_bumpMapFilePath);
        }

        star::command::transfer::SubmitTextureTransfer transfer{std::move(bumpMap)};
        context.getCmdBus().submit(transfer);
        const auto submission = transfer.getReply().get();

        m_bumpMap = submission.image;
        m_bumpMapTransfer = submission.semaphore;
    }

    if (m_bumpMap.isInitialized())
        registerTextureTransferWait(context, commandBuffer, m_bumpMap, m_bumpMapTransfer);

    TextureMaterial::prepRender(context, numFramesInFlight, frameBuilder, commandBuffer);
}

std::unique_ptr<star::StarShaderInfo> star::BumpMaterial::buildShaderInfo(core::device::DeviceContext &context,
                                                                          uint8_t numFramesInFlight,
                                                                          StarShaderInfo::Builder builder)
{
    assert(m_bumpMap.isInitialized() && "Underlying textures must be initialized before use");
    assert(m_textureHandle.isInitialized() && "Material needs to be prepared before it can be used");

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        builder.startOnFrameIndex(i);
        builder.startSet();
        builder.add(star::StarShaderInfo::TextureInfo{m_textureHandle, vk::ImageLayout::eShaderReadOnlyOptimal});
        builder.add(star::StarShaderInfo::TextureInfo{m_bumpMap, vk::ImageLayout::eShaderReadOnlyOptimal});
    }

    return builder.build();
}

std::vector<std::pair<vk::DescriptorType, const int>> star::BumpMaterial::getDescriptorRequests(
    int numFramesInFlight) const
{
    auto requests = TextureMaterial::getDescriptorRequests(numFramesInFlight);
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 2)};
}
