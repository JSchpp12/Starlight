#include "starlight/service/detail/screen_capture/SyncTargetRenderer.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::service::detail::screen_capture
{
using ListenStart = star::policy::ListenForStartOfNextFramePolicy<SyncTargetRenderer>;

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setCreatedOnFrameCount(uint64_t currentFrameCount)
{
    m_createdOnFrameCount = std::move(currentFrameCount);
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setSemaphore(vk::Semaphore semaphore)
{
    m_targetSemaphore = semaphore;
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setTargetFrameInFlightIndex(uint8_t index)
{
    m_targetFrameInFlightIndex = index;
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setDeviceEventBus(star::common::EventBus *eventBus)
{
    m_eventBus = eventBus;
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setDeviceCommandBufferManager(
    core::device::manager::ManagerCommandBuffer *commandBufferManager)
{
    m_commandBufferManager = commandBufferManager;
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setTargetCommandBuffer(Handle commandBuffer)
{
    m_targetCommandBuffer = std::move(commandBuffer);
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setSourceCommandBuffer(Handle commandBuffer)
{
    m_sourceCommandBuffer = std::move(commandBuffer);
    return *this;
}

SyncTargetRenderer::Builder &SyncTargetRenderer::Builder::setSemaphoreSignalValue(uint64_t signalValue)
{
    m_signalValue = std::move(signalValue);
    return *this;
}

std::shared_ptr<SyncTargetRenderer> SyncTargetRenderer::Builder::buildShared()
{
    assert(m_commandBufferManager != nullptr && m_targetSemaphore != VK_NULL_HANDLE &&
           m_commandBufferManager != nullptr && m_eventBus != nullptr && m_sourceCommandBuffer.isInitialized() &&
           m_targetCommandBuffer.isInitialized());
    assert(m_createdOnFrameCount.has_value());

    return std::shared_ptr<SyncTargetRenderer>(new SyncTargetRenderer(
        std::move(m_createdOnFrameCount.value()), std::move(m_signalValue), std::move(m_sourceCommandBuffer), std::move(m_targetCommandBuffer),
        m_targetFrameInFlightIndex, m_targetSemaphore, *m_commandBufferManager, *m_eventBus));
}

void SyncTargetRenderer::registerListener(star::common::EventBus &eventBus)
{
    std::shared_ptr<SyncTargetRenderer> sharedThis = shared_from_this();

    eventBus.subscribe(
        common::HandleTypeRegistry::instance().registerType(event::GetStartOfNextFrameTypeName),
        common::SubscriberCallbackInfo{
            [sharedThis](const star::common::IEvent &e, bool &keepAlive) { sharedThis->m_startOfFrameListener.eventCallback(e, keepAlive); },
            [sharedThis]() -> Handle * { return sharedThis->m_startOfFrameListener.getHandleForEventBus(); },
            [sharedThis](const Handle &noLongerNeededHandle) {
                sharedThis->m_startOfFrameListener.notificationFromEventBusOfDeletion(noLongerNeededHandle);
            }});
}

SyncTargetRenderer::SyncTargetRenderer(uint64_t createdOnFrameCount, uint64_t signalValue, Handle sourceCommandBuffer,
                                       Handle targetCommandBuffer, uint8_t targetFrameIndex,
                                       vk::Semaphore targetSemaphore,
                                       core::device::manager::ManagerCommandBuffer &commandBufferManager,
                                       star::common::EventBus &eventBus)
    : 
      m_createdOnFrameCount(std::move(createdOnFrameCount)), m_signalValue(std::move(signalValue)),
      m_sourceCommandBuffer(std::move(sourceCommandBuffer)), m_targetCommandBuffer(std::move(targetCommandBuffer)),
      m_targetFrameIndex(std::move(targetFrameIndex)), m_targetSemaphore(std::move(targetSemaphore)),
      m_commandBufferManager(commandBufferManager), m_eventBus(eventBus), m_startOfFrameListener(*this)
{
}

void SyncTargetRenderer::onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive)
{
    if (m_createdOnFrameCount < event.getFrameTracker().getCurrent().getGlobalFrameCounter() &&
        event.getFrameTracker().getCurrent().getFrameInFlightIndex() == m_targetFrameIndex)
    {
        m_commandBufferManager.get(m_targetCommandBuffer)
            .oneTimeWaitSemaphoreInfo.insert(m_sourceCommandBuffer, m_targetSemaphore,
                                             vk::PipelineStageFlagBits::eFragmentShader, m_signalValue);

        keepAlive = false;
    }
    else
    {
        keepAlive = true;
    }
}
} // namespace star::service::detail::screen_capture