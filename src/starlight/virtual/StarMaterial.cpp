#include "StarMaterial.hpp"
#include "core/device/DeviceContext.hpp"

void star::StarMaterial::prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight,
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

void star::StarMaterial::setResourceHandle(size_t setIndex, size_t bindingIndex, StarShaderInfo::ShaderInfo newHandle,
                                           std::optional<size_t> frameInFlightIndex)
{
    // m_resourceHandle is the material's primary resource, which MaterialService retires after a swap. A secondary
    // binding (e.g. BumpMaterial's map) must not overwrite it, or a later primary swap would retire the wrong
    // resource.
    const bool isPrimaryBinding = setIndex == 0 && bindingIndex == 0;

    if (newHandle.bufferInfo.has_value())
    {
        if (isPrimaryBinding)
            m_resourceHandle = newHandle.bufferInfo->handle.value_or(Handle());
        if (shaderInfo)
            shaderInfo->setNewResource(setIndex, bindingIndex, std::move(newHandle.bufferInfo.value()),
                                       frameInFlightIndex);
    }
    else if (newHandle.textureInfo.has_value())
    {
        if (isPrimaryBinding)
            m_resourceHandle = newHandle.textureInfo->handle;
        if (shaderInfo)
            shaderInfo->setNewResource(setIndex, bindingIndex, std::move(newHandle.textureInfo.value()),
                                       frameInFlightIndex);
    }
}

void star::StarMaterial::cleanupRender(core::device::DeviceContext &context)
{
    if (shaderInfo)
        shaderInfo->cleanupRender(context.getDevice());
}

std::optional<star::StarShaderInfo::ShaderInfo>
star::StarMaterial::getResourceInfo(size_t setIndex, size_t bindingIndex,
                                    std::optional<size_t> frameInFlightIndex) const
{
    if (!shaderInfo)
        return std::nullopt;

    const size_t frame = frameInFlightIndex.value_or(0);
    const auto &frames = shaderInfo->getShaderInfoSets();
    if (frame >= frames.size())
        return std::nullopt;

    const auto &sets = frames[frame];
    if (setIndex >= sets.size() || sets[setIndex] == nullptr)
        return std::nullopt;

    const auto &slots = sets[setIndex]->slots;
    if (bindingIndex >= slots.size() || !slots[bindingIndex].has_value())
        return std::nullopt;

    return slots[bindingIndex];
}

void star::StarMaterial::bind(vk::CommandBuffer &commandBuffer, vk::PipelineLayout pipelineLayout,
                              int swapChainImageIndex, uint32_t firstSetIndex)
{
    std::array<vk::DescriptorSet, 1> descriptors{};
    assert(this->shaderInfo->getNumDescriptorSets(swapChainImageIndex) <= descriptors.size());
    size_t numWritten{0};

    this->shaderInfo->getDescriptors(swapChainImageIndex, descriptors.data(), numWritten);
    if (numWritten != 0)
    {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, firstSetIndex, numWritten,
                                         descriptors.data(), 0, nullptr);
    }
}

bool star::StarMaterial::isKnownToBeReady(uint8_t frameInFlightIndex)
{
    return this->shaderInfo->isReady(frameInFlightIndex);
}

std::vector<std::pair<vk::DescriptorType, const int>> star::StarMaterial::getDescriptorRequests(
    int numFramesInFlight) const
{
    return std::vector<std::pair<vk::DescriptorType, const int>>();
}

std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>> star::StarMaterial::getDataSemaphores(
    uint8_t frameInFlightIndex) const
{
    auto semaphoreInfo = std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>>();

    return semaphoreInfo;
}

std::unique_ptr<star::StarShaderInfo> star::StarMaterial::buildShaderInfo(core::device::DeviceContext &device,
                                                                          uint8_t numFramesInFlight,
                                                                          StarShaderInfo::Builder builder)
{
    return builder.build();
}
