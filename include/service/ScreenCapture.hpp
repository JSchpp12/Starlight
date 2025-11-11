#pragma once

#include "Handle.hpp"
#include "core/device/DeviceContext.hpp"
#include "service/InitParameters.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"


namespace star::service
{
class ScreenCapture
{
  public:
    ScreenCapture(std::vector<StarTextures::Texture> targetTextures, std::vector<Handle> textureReadySemaphore)
        : m_targetTextures(std::move(targetTextures)), m_targetTexturesReadySemaphores(std::move(textureReadySemaphore))
    {
    }

    void init(const uint8_t &numFramesInFlight);

    void setInitParameters(InitParameters &params);

    void shutdown();

  private:
    struct DeviceInfo
    {
        core::device::StarDevice *device = nullptr;
        core::RenderingSurface *surface = nullptr;
        core::device::system::EventBus *eventBus = nullptr;
        job::TaskManager *taskManager = nullptr;
    };

    Handle m_subscriberHandle;
    std::vector<StarTextures::Texture> m_targetTextures;
    std::vector<Handle> m_targetTexturesReadySemaphores;
    std::vector<StarTextures::Texture> m_transferDstTextures;
    std::vector<StarBuffers::Buffer> m_hostVisibleBuffers;
    DeviceInfo m_deviceInfo;

    virtual Handle registerCommandBuffer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    std::vector<StarTextures::Texture> createTransferDstTextures(core::device::StarDevice &device,
                                                                 const uint8_t &numFramesInFlight,
                                                                 const vk::Extent2D &renderingResolution) const;

    void initBuffers(const uint8_t &numFramesInFlight);

    std::vector<StarBuffers::Buffer> createHostVisibleBuffers(core::device::StarDevice &device,
                                                              const uint8_t &numFramesInFlight,
                                                              const vk::Extent2D &renderingResolution,
                                                              const vk::DeviceSize &size) const;

    void trigger();

    void eventCallback(const star::common::IEvent &e, bool &keepAlive);

    Handle *notificationFromEventBusGetHandle();

    void notificationFromEventBusDeleteHandle(const Handle &handle);

    void cleanupBuffers(core::device::DeviceContext &context);

    void cleanupIntermediateImages(core::device::DeviceContext &context);

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                               const uint64_t &frameIndex) const;

    void registerWithEventBus(core::device::system::EventBus &eventBus);
};
} // namespace star::service