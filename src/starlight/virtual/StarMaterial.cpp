#include "StarMaterial.hpp"

void star::StarMaterial::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                    star::StarShaderInfo::Builder frameBuilder, star::Handle commandBuffer)
{
    // Only texture-bearing materials register transfer waits against the provided
    // command buffer; the base has nothing to wait on.
    (void)commandBuffer;

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
                              int swapChainImageIndex, uint32_t firstSetIndex)
{
    std::vector<vk::DescriptorSet> descriptors{4};
    size_t numWritten{0};

    this->shaderInfo->getDescriptors(swapChainImageIndex, descriptors.data(), numWritten);
    if (numWritten != 0)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, firstSetIndex,
                                         descriptors.size(), descriptors.data(), 0, nullptr);
    }
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

std::unique_ptr<star::StarShaderInfo> star::StarMaterial::buildShaderInfo(core::device::DeviceContext &device,
                                                                          const uint8_t &numFramesInFlight,
                                                                          StarShaderInfo::Builder builder)
{
    return builder.build();
}
