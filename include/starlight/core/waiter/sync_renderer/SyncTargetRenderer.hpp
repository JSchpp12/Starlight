#pragma once

#include "starlight/core/device/managers/ManagerCommandBuffer.hpp"
#include "starlight/policy/ListenForStartOfNextFramePolicy.hpp"

#include <star_common/EventBus.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::core::waiter::sync_renderer
{
class SyncTargetRenderer : public std::enable_shared_from_this<SyncTargetRenderer>
{
  public:
    SyncTargetRenderer(star::common::EventBus &eventBus,
                       core::device::manager::ManagerCommandBuffer &commandBufferManager, Handle sourceCommandBuffer,
                       Handle targetCommandBuffer, vk::Semaphore targetSemaphore, uint64_t createdOnFrameCount,
                       uint8_t targetFrameIndex)
        : m_sourceCommandBuffer(std::move(sourceCommandBuffer)), m_targetCommandBuffer(std::move(targetCommandBuffer)),
          m_targetSemaphore(std::move(targetSemaphore)), m_eventBus(eventBus),
          m_commandBufferManager(commandBufferManager), m_targetFrameIndex(std::move(targetFrameIndex)),
          m_createdOnFrameCount(std::move(createdOnFrameCount)), m_startOfFrameListener(*this)
    {
    }

    virtual ~SyncTargetRenderer()
    {
        cleanup();
    }

    void init()
    {
        registerListener(m_eventBus);
    }

    void cleanup()
    {
        m_startOfFrameListener.cleanup(m_eventBus);
    }

    void onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive);

  protected:
    Handle m_sourceCommandBuffer;
    Handle m_targetCommandBuffer;
    vk::Semaphore m_targetSemaphore = VK_NULL_HANDLE;
    star::common::EventBus &m_eventBus;
    core::device::manager::ManagerCommandBuffer &m_commandBufferManager;

    virtual void registerWaitWithManager() = 0;

  private:
    uint8_t m_targetFrameIndex;
    uint64_t m_createdOnFrameCount;
    star::policy::ListenForStartOfNextFramePolicy<SyncTargetRenderer> m_startOfFrameListener;

    void registerListener(star::common::EventBus &eventBus);
};
} // namespace star::core::waiter::sync_renderer