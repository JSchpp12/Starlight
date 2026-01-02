#pragma once

#include <starlight/core/device/managers/ManagerCommandBuffer.hpp>
#include <starlight/policy/ListenForPrepForNextFramePolicy.hpp>

#include <star_common/EventBus.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::service::detail::screen_capture
{
class SyncTargetRenderer : public std::enable_shared_from_this<SyncTargetRenderer>,
                           private star::policy::ListenForPrepForNextFramePolicy<SyncTargetRenderer>
{
  public:
    class Builder
    {
      public:
        Builder() = default;
        Builder &setCreatedOnFrameCount(uint64_t currentFrameCounter);
        Builder &setSemaphore(vk::Semaphore semaphore);
        Builder &setTargetFrameInFlightIndex(uint8_t index);
        Builder &setDeviceCommandBufferManager(core::device::manager::ManagerCommandBuffer *commandBufferManager);
        Builder &setDeviceEventBus(star::common::EventBus *eventBus);
        Builder &setTargetCommandBuffer(Handle commandBuffer);
        Builder &setSourceCommandBuffer(Handle commandBuffer);
        std::shared_ptr<SyncTargetRenderer> buildShared();

      private:
        std::optional<uint64_t> m_createdOnFrameCount = std::nullopt;
        Handle m_targetCommandBuffer;
        Handle m_sourceCommandBuffer;
        uint8_t m_targetFrameInFlightIndex;
        vk::Semaphore m_targetSemaphore = VK_NULL_HANDLE;
        core::device::manager::ManagerCommandBuffer *m_commandBufferManager = nullptr;
        star::common::EventBus *m_eventBus = nullptr;
    };

    ~SyncTargetRenderer()
    {
        cleanup();
    }

    void init()
    {
        registerListener(m_eventBus);
    }

    void cleanup()
    {
        star::policy::ListenForPrepForNextFramePolicy<SyncTargetRenderer>::cleanup(m_eventBus);
    }

  protected:
    void onPrepForNextFrame(const event::PrepForNextFrame &event, bool &keepAlive);

  private:
    friend class star::policy::ListenForPrepForNextFramePolicy<SyncTargetRenderer>;
    uint64_t m_createdOnFrameCount;
    Handle m_sourceCommandBuffer;
    Handle m_targetCommandBuffer;
    uint8_t m_targetFrameIndex;
    vk::Semaphore m_targetSemaphore = VK_NULL_HANDLE;
    core::device::manager::ManagerCommandBuffer &m_commandBufferManager;
    star::common::EventBus &m_eventBus;

    void registerListener(star::common::EventBus &eventBus);

    SyncTargetRenderer(uint64_t createdOnFrameCount, Handle sourceCommandBuffer, Handle targetCommandBuffer, uint8_t targetFrameIndex, vk::Semaphore targetSemaphore,
                       core::device::manager::ManagerCommandBuffer &commandBufferManager,
                       star::common::EventBus &eventBus);
};
} // namespace star::service::detail::screen_capture