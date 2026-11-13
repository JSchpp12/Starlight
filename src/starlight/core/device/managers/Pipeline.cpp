#include "core/device/managers/Pipeline.hpp"

#include "core/logging/LoggingFactory.hpp"

namespace star::core::device::manager
{
void PipelineRecord::resolve() const
{
    if (builtPipeline.isRenderReady() || !pending.has_value() || !pending->isReady())
        return;

    auto result = pending->take();
    pending.reset();

    if (result.has_value())
        builtPipeline = std::move(*result);
    else
        core::logging::warning("Pipeline build failed: ", result.error().what());
}

void PipelineRecord::waitForReady() const
{
    if (!pending.has_value())
        return;

    pending->wait();
    resolve();
}

void PipelineRecord::cleanupRender(core::device::StarDevice &device)
{
    if (pending.has_value())
    {
        // An unpublishing build is owned by its producer; it must publish (or be destroyed) before teardown.
        if (!pending->isReady())
            return;

        resolve();
    }

    builtPipeline.destroy(device.getVulkanDevice());
}

void Pipeline::init(device::StarDevice *device, common::EventBus &eventBus, job::TaskManager &taskSystem)
{
    TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50, PipelineResourceTypeName>::init(device, eventBus,
                                                                                                    taskSystem);
}

void Pipeline::submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                          common::EventBus &eventBus, PipelineRecord *storedRecord)
{
    // Build submission is state-driven from ProcessPipelinesWhichAreNowReadyForBuild, called after this record is
    // submitted and after each shader compile completes.
    (void)device;
    (void)handle;
    (void)taskSystem;
    (void)eventBus;
    (void)storedRecord;
}
} // namespace star::core::device::manager
