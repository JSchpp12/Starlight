#include "StarMaterial.hpp"

void star::StarMaterial::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                    star::StarShaderInfo::Builder frameBuilder)
{
    if (!shaderInfo)
    {
        shaderInfo = buildShaderInfo(context, numFramesInFlight, std::move(frameBuilder));
    }
}

void star::StarMaterial::cleanupRender(core::device::DeviceContext &context)
{
    shaderInfo->cleanupRender(context.getDevice());
}

void star::StarMaterial::bind(vk::CommandBuffer &commandBuffer, vk::PipelineLayout pipelineLayout,
                              int swapChainImageIndex)
{
    // bind the descriptor sets for the given image index
    auto descriptors = this->shaderInfo->getDescriptors(swapChainImageIndex);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptors.size(),
                                     descriptors.data(), 0, nullptr);
}

bool star::StarMaterial::isKnownToBeReady(const uint8_t &frameInFlightIndex)
{
    return this->shaderInfo->isReady(frameInFlightIndex);
}

std::vector<std::pair<vk::DescriptorType, const int>> star::StarMaterial::getDescriptorRequests(
    const int &numFramesInFlight) const
{
    return std::vector<std::pair<vk::DescriptorType, const int>>();
}

std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>> star::StarMaterial::getDataSemaphores(
    const uint8_t &frameInFlightIndex) const
{
    auto semaphoreInfo = std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>>();

    return semaphoreInfo;
}