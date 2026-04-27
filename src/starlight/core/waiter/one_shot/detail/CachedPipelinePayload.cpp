#include "starlight/core/waiter/one_shot/detail/CachedPipelinePayload.hpp"

#include "starlight/event/PipelineReady.hpp"

namespace star::core::waiter::one_shot
{
void CachedPipelinePayload::operator()(const star::common::IEvent &e, bool &keepAlive)
{
    const auto &event = static_cast<const star::event::PipelineReady &>(e);
    if (event.getTriggeringPipeline() == targetPipelineRegistration)
    {
        assert(pipelineManager != nullptr && "Pointer to valid manager must be provided on creation");
        assert(pipelineToSet != nullptr && "Parent pipeline pointer must be valid");

        *pipelineToSet = pipelineManager->get(targetPipelineRegistration)->request.pipeline.getVulkanPipeline();

        keepAlive = false;
    }
    else
    {
        keepAlive = true;
    }
}
} // namespace star::core::waiter::one_shot