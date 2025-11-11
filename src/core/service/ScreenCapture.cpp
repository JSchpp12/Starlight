#include "service/ScreenCapture.hpp"

#include "event/TriggerScreenshot.hpp"
#include "logging/LoggingFactory.hpp"

namespace star::service
{

// void ScreenCapture::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
//                                const vk::Extent2D &renderingResolution)
// {
//     m_transferDstTextures = createTransferDstTextures(context, numFramesInFlight, renderingResolution);
//     m_hostVisibleBuffers = createHostVisibleBuffers(context, numFramesInFlight, renderingResolution,
//                                                     m_transferDstTextures.front().getSize());
// }

// void ScreenCapture::cleanupRender(core::device::DeviceContext &context)
// {
//     cleanupIntermediateImages(context);
//     cleanupBuffers(context);
// }

void ScreenCapture::init(const uint8_t &numFramesInFlight)
{
    assert(m_deviceInfo.eventBus != nullptr && "Event bus must be initialized");
    registerWithEventBus(*m_deviceInfo.eventBus);

    initBuffers(numFramesInFlight);
}

void ScreenCapture::initBuffers(const uint8_t &numFramesInFlight)
{
    assert(m_deviceInfo.device != nullptr && "Device must be initialized"); 
    assert(m_deviceInfo.surface != nullptr && "Surface must be initialized"); 
    assert(m_deviceInfo.taskManager != nullptr && "Task manager must be initalized"); 

        // createHostVisibleBuffers(device, )
}

void ScreenCapture::setInitParameters(InitParameters &params)
{
    m_deviceInfo = DeviceInfo{.device = &params.device,
                              .surface = &params.surface,
                              .eventBus = &params.eventBus,
                              .taskManager = &params.taskManager};
}

void ScreenCapture::shutdown()
{
}

star::Handle ScreenCapture::registerCommandBuffer(core::device::DeviceContext &context,
                                                  const uint8_t &numFramesInFlight)
{
    return Handle();
}

std::vector<StarTextures::Texture> ScreenCapture::createTransferDstTextures(
    core::device::StarDevice &device, const uint8_t &numFramesInFlight, const vk::Extent2D &renderingResolution) const
{
    auto textures = std::vector<StarTextures::Texture>();
    const vk::Format format = m_targetTextures.front().getBaseFormat();

    const auto queueFamilyInds =
        std::vector<uint32_t>{device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                              device.getDefaultQueue(star::Queue_Type::Ttransfer).getParentQueueFamilyIndex()};

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        textures.emplace_back(
            StarTextures::Texture::Builder(device.getVulkanDevice(), device.getAllocator().get())
                .setBaseFormat(format)
                .setCreateInfo(
                    Allocator::AllocationBuilder()
                        .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                        .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                        .build(),
                    vk::ImageCreateInfo()
                        .setExtent(vk::Extent3D()
                                       .setWidth(renderingResolution.width)
                                       .setHeight(renderingResolution.height)
                                       .setDepth(1))
                        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc)
                        .setImageType(vk::ImageType::e2D)
                        .setMipLevels(1)
                        .setArrayLayers(1)
                        .setTiling(vk::ImageTiling::eOptimal)
                        .setInitialLayout(vk::ImageLayout::eUndefined)
                        .setSamples(vk::SampleCountFlagBits::e1)
                        .setSharingMode(vk::SharingMode::eConcurrent)
                        .setQueueFamilyIndexCount(2)
                        .setPQueueFamilyIndices(queueFamilyInds.data()),
                    "ScreenCapture_IntermediateTransferTexture")
                .build());
    }

    return textures;
}

std::vector<StarBuffers::Buffer> ScreenCapture::createHostVisibleBuffers(core::device::StarDevice &device,
                                                                         const uint8_t &numFramesInFlight,
                                                                         const vk::Extent2D &renderingResolution,
                                                                         const vk::DeviceSize &bufferSize) const
{
    auto buffers = std::vector<StarBuffers::Buffer>();

    auto builder = StarBuffers::Buffer::Builder(device.getAllocator().get())
                       .setAllocationCreateInfo(Allocator::AllocationBuilder()
                                                    .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                                              VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                                    .setUsage(VMA_MEMORY_USAGE_AUTO_PREFER_HOST)
                                                    .build(),
                                                vk::BufferCreateInfo()
                                                    .setSharingMode(vk::SharingMode::eExclusive)
                                                    .setSize(bufferSize)
                                                    .setUsage(vk::BufferUsageFlagBits::eTransferDst),
                                                "ScreenCapture_DstBuffer")
                       .setInstanceCount(uint32_t{1})
                       .setInstanceSize(bufferSize);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        buffers.emplace_back(builder.build());
    }

    return buffers;
}

void ScreenCapture::trigger()
{
    core::logging::log(boost::log::trivial::info, "Trigger screenshot");
}

void ScreenCapture::cleanupBuffers(core::device::DeviceContext &context)
{
    for (auto &buffer : m_hostVisibleBuffers)
    {
        buffer.cleanupRender(context.getDevice().getVulkanDevice());
    }
}

void ScreenCapture::cleanupIntermediateImages(core::device::DeviceContext &context)
{
    for (auto &image : m_transferDstTextures)
    {
        image.cleanupRender(context.getDevice().getVulkanDevice());
    }
}

void ScreenCapture::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                        const uint64_t &frameIndex)
{
    // commandBuffer
}

void ScreenCapture::addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                          const uint64_t &frameIndex) const
{
}

void ScreenCapture::registerWithEventBus(core::device::system::EventBus &eventBus)
{
    eventBus.subscribe<event::TriggerScreenshot>(
        {[this](const star::common::IEvent &e, bool &keepAlive) { this->eventCallback(e, keepAlive); },
         [this]() -> Handle * { return this->notificationFromEventBusGetHandle(); },
         [this](const Handle &handle) { this->notificationFromEventBusDeleteHandle(handle); }});
}

void ScreenCapture::eventCallback(const star::common::IEvent &e, bool &keepAlive)
{
    const auto &screenEvent = static_cast<const event::TriggerScreenshot &>(e);
    trigger();
    keepAlive = true;
}

star::Handle *ScreenCapture::notificationFromEventBusGetHandle()
{
    return &m_subscriberHandle;
}

void ScreenCapture::notificationFromEventBusDeleteHandle(const Handle &handle)
{
}
} // namespace star::service