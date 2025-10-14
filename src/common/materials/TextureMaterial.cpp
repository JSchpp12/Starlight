#include "TextureMaterial.hpp"

#include "ManagerRenderResource.hpp"
#include "StarShaderInfo.hpp"
#include "TransferRequest_CompressedTextureFile.hpp"
#include "TransferRequest_TextureFile.hpp"

#include "FileHelpers.hpp"
#include "ManagerController_RenderResource_TextureFile.hpp"

#include <cassert>

void star::TextureMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const
{
    constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler,
                            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

star::TextureMaterial::TextureMaterial(std::string texturePath) : m_texturePath(std::move(texturePath))
{
    if (!file_helpers::FileExists(m_texturePath))
    {
        throw std::runtime_error("Provided texture path for material does not exist");
    }
}

void star::TextureMaterial::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                       star::StarShaderInfo::Builder frameBuilder)
{
    assert(!m_textureHandle.isInitialized() && "Should not be prepared for render more than once");

    {
        const auto texSemaphore = context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest(false));
        const auto graphicsIndex =
            context.getDevice().getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex();
        const auto deviceProperties = context.getDevice().getPhysicalDevice().getProperties();

        auto texture = std::unique_ptr<TransferRequest::Texture>();
        if (TransferRequest::CompressedTextureFile::IsFileCompressedTexture(m_texturePath))
        {
            texture = std::make_unique<TransferRequest::CompressedTextureFile>(std::move(graphicsIndex), std::move(deviceProperties), std::make_shared<SharedCompressedTexture>(m_texturePath));
        }
        else
        {
            texture = std::make_unique<TransferRequest::TextureFile>(std::move(graphicsIndex), std::move(deviceProperties), m_texturePath);
        }

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
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 1 * numFramesInFlight)};
}
