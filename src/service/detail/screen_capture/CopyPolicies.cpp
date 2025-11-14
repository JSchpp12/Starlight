#include "service/detail/screen_capture/CopyPolicies.hpp"

#include "Enums.hpp"
#include "core/device/managers/Semaphore.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "logging/LoggingFactory.hpp"

namespace star::service::detail::screen_capture
{
void DefaultCopyPolicy::init(core::device::system::EventBus &eventBus, const uint8_t &numFramesInFlight)
{
    m_doneSemaphores = createSemaphores(eventBus, numFramesInFlight);
}

void DefaultCopyPolicy::triggerSubmission(core::device::manager::ManagerCommandBuffer &commandManager)
{
    commandManager.submitDynamicBuffer(m_commandBuffer);
}

void DefaultCopyPolicy::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                            const uint64_t &frameIndex)
{
    core::logging::log(boost::log::trivial::info, "Beginnign record"); 
    
}

void DefaultCopyPolicy::addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                              const uint64_t &frameIndex)
{
}

void DefaultCopyPolicy::registerWithCommandBufferManager(core::device::StarDevice &device,
                                                         core::device::manager::ManagerCommandBuffer &commandManager,
                                                         const uint64_t &currentFrameIndex)
{
    m_commandBuffer = commandManager.submit(
        device, currentFrameIndex,
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
        eventBus.emit(
            core::device::system::event::ManagerRequest{result[i], core::device::manager::SemaphoreRequest{true}});
    }

    return result;
}
} // namespace star::service::detail::screen_capture