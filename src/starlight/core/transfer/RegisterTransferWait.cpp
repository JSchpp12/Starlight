#include "starlight/core/transfer/RegisterTransferWait.hpp"

#include "starlight/command/command_order/ForceCPURecordingWait.hpp"
#include "starlight/core/device/DeviceContext.hpp"

#include <utility>

namespace star::core::transfer
{
void RegisterTransferWait(core::device::DeviceContext &context, Handle pass, Handle resource, vk::Semaphore semaphore,
                          uint64_t signalValue, vk::PipelineStageFlags stages)
{
    if (!pass.isInitialized() || !resource.isInitialized() || semaphore == VK_NULL_HANDLE)
        return;

    context.getManagerCommandBuffer().m_manager.get(pass).oneTimeWaitSemaphoreInfo.insert(resource, semaphore, stages,
                                                                                          signalValue);

    // The pass must not submit its timeline wait until the transfer resource is CPU-published. The waiter runs at the
    // pass's record site, before the record callback and before submission.
    star::command_order::ForceCPURecordingWait command{pass};
    command.addResource(std::move(resource));
    context.getCmdBus().submit(command);
}
} // namespace star::core::transfer
