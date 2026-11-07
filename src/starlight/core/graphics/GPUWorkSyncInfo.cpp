#include "starlight/core/graphics/GPUWorkSyncInfo.hpp"

#include "core/device/DeviceContext.hpp"
#include "starlight/command/command_order/GetPassInfo.hpp"

namespace star::core::graphics
{
RecordingSubmissionInfo GetRecordingSubmissionInfo(core::device::DeviceContext &device,
                                                   const Handle &recordingCommandBuffer)
{
    // The value the recording submission will signal once it completes. Object-specific one-shot releases are gated
    // on this so worker-owned transients are not freed while the recorded work is still executing on the GPU.
    command_order::GetPassInfo info{recordingCommandBuffer};
    device.getCmdBus().submit(info);
    const auto &reply = info.getReply().get();

    return RecordingSubmissionInfo{
        .gate = SemaphoreInfo{.signalValue = reply.toSignalValue, .semaphore = reply.signaledSemaphore},
    };
}
} // namespace star::core::graphics
