#include "BumpMaterial.hpp"

#include "ManagerController_RenderResource_TextureFile.hpp"

void star::BumpMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const
{
    constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::BumpMaterial::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder)
{
    auto bumpSemaphore = context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest(false)); 

    m_bumpMap = ManagerRenderResource::addRequest(
        context.getDeviceID(), 
        context.getSemaphoreManager().get(bumpSemaphore)->semaphore,
        std::make_unique<star::ManagerController::RenderResource::TextureFile>(m_bumpMapFilePath));

    TextureMaterial::prepRender(context, numFramesInFlight, frameBuilder);
}

std::unique_ptr<star::StarShaderInfo> star::BumpMaterial::buildShaderInfo(core::device::DeviceContext &context, const uint8_t &numFramesInFlight, StarShaderInfo::Builder builder)
{
	assert(m_bumpMap.isInitialized() && "Underlying textures must be initialized before use"); 
	assert(m_textureHandle.isInitialized() && "Material needs to be prepared before it can be used"); 

    for (uint8_t i = 0; i < numFramesInFlight; i++){
        builder.startOnFrameIndex(i); 
        builder.add(m_textureHandle, vk::ImageLayout::eShaderReadOnlyOptimal);
        builder.add(m_bumpMap, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    return builder.build(); 
}

std::vector<std::pair<vk::DescriptorType, const int>> star::BumpMaterial::getDescriptorRequests(
    const int &numFramesInFlight) const
{
    auto requests = TextureMaterial::getDescriptorRequests(numFramesInFlight); 
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 2)};
}
