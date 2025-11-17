#include "service/detail/screen_capture/CopyPolicies.hpp"

#include "Enums.hpp"
#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/device/system/event/StartOfNextFrame.hpp"
#include "logging/LoggingFactory.hpp"

namespace star::service::detail::screen_capture
{
void DefaultCopyPolicy::init(DeviceInfo &deviceInfo, const uint8_t &numFramesInFlight)
{
    m_deviceInfo = &deviceInfo;
    m_doneSemaphores = createSemaphores(*m_deviceInfo->eventBus, numFramesInFlight);
}

void DefaultCopyPolicy::triggerSubmission(CalleeRenderDependencies &targetDeps)
{
    m_deviceInfo->commandManager->submitDynamicBuffer(m_commandBuffer);
    registerListenerForNextFrameStart(targetDeps);
}

void DefaultCopyPolicy::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                            const uint64_t &frameIndex)
{
    core::logging::log(boost::log::trivial::info, "Start record");
}

void DefaultCopyPolicy::addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                              const uint64_t &frameIndex)
{
}

void DefaultCopyPolicy::registerWithCommandBufferManager()
{
    m_commandBuffer = m_deviceInfo->commandManager->submit(
        *m_deviceInfo->device, *m_deviceInfo->currentFrameCounter,
        core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&DefaultCopyPolicy::recordCommandBuffer, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::after_presentation,
            .orderIndex = Command_Buffer_Order_Index::dont_care,
            .type = Queue_Type::Ttransfer,
            .waitStage = vk::PipelineStageFlagBits::eTransfer,
            .willBeSubmittedEachFrame = false,
            .overrideBufferSubmissionCallback =
                std::bind(&DefaultCopyPolicy::submitBuffer, this, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)});
}

vk::Semaphore DefaultCopyPolicy::submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                                              std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                              std::vector<vk::Semaphore> dataSemaphores,
                                              std::vector<vk::PipelineStageFlags> dataWaitPoints)
{
    return vk::Semaphore();
}

std::vector<Handle> DefaultCopyPolicy::createSemaphores(core::device::system::EventBus &eventBus,
                                                        const uint8_t &numFramesInFlight)
{
    auto result = std::vector<Handle>(numFramesInFlight);

    for (uint8_t i = 0; i < numFramesInFlight; i++)
    {
        eventBus.emit(core::device::system::event::ManagerRequest{
            common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::SemaphoreTypeName()),
            core::device::manager::SemaphoreRequest{true}, result[i]});
    }

    return result;
}

void DefaultCopyPolicy::registerListenerForNextFrameStart(CalleeRenderDependencies &deps)
{
    Handle calleeHandle = deps.commandBufferContainingTarget;
    m_deviceInfo->eventBus->subscribe(
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
            core::device::system::event::StartOfNextFrameName()),
        {[this, calleeHandle](const star::common::IEvent &e, bool &keepAlive) {
             this->startOfFrameEventCallback(calleeHandle, e, keepAlive);
         },
         [this]() -> Handle * { return &this->m_startOfFrameListener; },
         [this](const Handle &noLongerNeededHandle) { this->m_startOfFrameListener = Handle(); }});
}

void DefaultCopyPolicy::startOfFrameEventCallback(const Handle &calleeCommandBuffer, const star::common::IEvent &e,
                                                  bool &keepAlive)
{
    assert(calleeCommandBuffer.isInitialized() && "Callee information must be provided");

    const auto &startEvent = static_cast<const core::device::system::event::StartOfNextFrame &>(e);
    m_deviceInfo->commandManager->get(calleeCommandBuffer)
        .oneTimeWaitSemaphoreInfo.insert(m_doneSemaphores.front(),
                                         m_deviceInfo->semaphoreManager->get(m_doneSemaphores.front())->semaphore,
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput);
    keepAlive = false;
}
} // namespace star::service::detail::screen_capture