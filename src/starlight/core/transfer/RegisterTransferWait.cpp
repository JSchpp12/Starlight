#include "starlight/core/transfer/RegisterTransferWait.hpp"

#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/managers/Image.hpp"
#include "starlight/command/command_order/ForceCPURecordingWait.hpp"
#include "starlight/core/device/DeviceContext.hpp"
#include "starlight/core/transfer/ReadinessCondition.hpp"
#include "starlight/setup/RenderResourceSetup.hpp"

#include <star_common/Handle.hpp>

#include <utility>

namespace star::core::transfer
{
void RegisterTransferWait(core::device::DeviceContext &context, Handle pass, Handle resource, Handle semaphoreRecord,
                          vk::PipelineStageFlags stages)
{
    if (!pass.isInitialized() || !resource.isInitialized() || !semaphoreRecord.isInitialized())
        return;

    context.getManagerCommandBuffer().m_manager.get(pass).oneTimeWaitSemaphoreInfo.registerWait(semaphoreRecord,
                                                                                                stages);

    // Frame-wide transfer waits are drained by ManagerCommandBuffer immediately before the finalization phase
    // records; an unset pass marks this wait for that container.
    star::command_order::ForceCPURecordingWait command{};
    command.addResource(resource);
    context.getCmdBus().submit(command);
}

void RegisterPassConditions(core::device::DeviceContext &context, Handle pass,
                            std::vector<ReadinessCondition> conditions, ReadinessFailurePolicy onFailure)
{
    if (!pass.isInitialized() || conditions.empty())
        return;

    for (const auto &condition : conditions)
    {
        if (!condition.semaphoreRecord.has_value())
            continue;

        context.getManagerCommandBuffer().m_manager.get(pass).oneTimeWaitSemaphoreInfo.registerWait(
            *condition.semaphoreRecord, condition.waitStages);
    }

    star::command_order::ForceCPURecordingWait command{pass};
    command.setFailurePolicy(onFailure);
    for (auto &condition : conditions)
        command.addCondition(std::move(condition));

    context.getCmdBus().submit(command);
}

ReadinessCondition MakeTransferReadinessCondition(core::device::DeviceContext &context, const setup::TransferWait &wait)
{
    auto &managers = context.getGraphicsManagers();

    ReadinessCondition condition{};
    condition.resource = wait.key;

    if (wait.key.getType() == managers.imageManager.getHandleType())
        condition.readiness = ImageReadiness{&managers.imageManager};
    else if (wait.key.getType() == managers.bufferManager.getHandleType())
        condition.readiness = BufferReadiness{&managers.bufferManager};
    else
        condition.readiness = RenderResourceBufferReadiness{context.getDeviceID()};

    // An absent record means the transfer has no GPU timeline to wait on; leave `semaphoreRecord` unset rather than
    // staging an empty handle.
    if (wait.semaphoreRecord.isInitialized())
        condition.semaphoreRecord = wait.semaphoreRecord;
    condition.waitStages = wait.stages;
    return condition;
}
} // namespace star::core::transfer
