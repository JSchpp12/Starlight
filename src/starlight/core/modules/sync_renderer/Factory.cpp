#include "starlight/core/modules/sync_renderer/Factory.hpp"

#include "starlight/core/modules/sync_renderer/SyncTargetRendererBinary.hpp"
#include "starlight/core/modules/sync_renderer/SyncTargetRendererTimeline.hpp"

namespace star::core::modules::sync_renderer
{
Factory::Factory(star::common::EventBus &eventBus, core::device::manager::ManagerCommandBuffer &commandBufferManager)
    : m_eventBus(eventBus), m_commandBufferManager(commandBufferManager)
{
}

Factory &Factory::setCreatedOnFrameCount(uint64_t currentFrameCounter)
{
    m_createdOnFrameCount = std::move(currentFrameCounter);
    return *this;
}

Factory &Factory::setSemaphore(vk::Semaphore semaphore)
{
    m_targetSemaphore = semaphore;
    return *this;
}

Factory &Factory::setSemaphoreSignalValue(uint64_t signalValue)
{
    m_signalValue = std::move(signalValue);
    return *this;
}

Factory &Factory::setTargetFrameInFlightIndex(uint8_t index)
{
    m_targetFrameInFlightIndex = index;
    return *this;
}

Factory &Factory::setTargetCommandBuffer(Handle commandBuffer)
{
    m_targetCommandBuffer = std::move(commandBuffer);
    return *this;
}

Factory &Factory::setWaitPipelineStage(vk::PipelineStageFlags flags)
{
    m_flags = std::move(flags);
    return *this;
}

Factory &Factory::setSourceCommandBuffer(Handle commandBuffer)
{
    m_sourceCommandBuffer = std::move(commandBuffer);
    return *this;
}

std::shared_ptr<SyncTargetRenderer> Factory::build()
{
    std::shared_ptr<SyncTargetRenderer> waiter;

    if (m_signalValue.has_value())
    {
        waiter = std::make_shared<SyncTargetRendererTimeline>(
            m_signalValue.value(), m_eventBus, m_commandBufferManager, std::move(m_sourceCommandBuffer),
            std::move(m_targetCommandBuffer), std::move(m_targetSemaphore), m_createdOnFrameCount.value(),
            std::move(m_targetFrameInFlightIndex));
    }
    else
    {
        waiter = std::make_shared<SyncTargetRendererBinary>(
            m_eventBus, m_commandBufferManager, std::move(m_sourceCommandBuffer), std::move(m_targetCommandBuffer),
            std::move(m_targetSemaphore), m_createdOnFrameCount.value(), std::move(m_targetFrameInFlightIndex));
    }

    waiter->init();

    return waiter;
}
} // namespace star::core::modules::sync_renderer