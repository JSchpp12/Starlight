#include "complete_tasks/TaskFactory.hpp"

#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"

#include "core/device/managers/GraphicsContainer.hpp"
#include <star_common/EventBus.hpp>
#include "core/device/system/event/ShaderCompiled.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#pragma region BuildPipeline

void star::job::complete_tasks::task_factory::ExecuteBuildPipelineComplete(void *device, void *taskSystem,
                                                                           void *eventBus, void *graphicsManagers,
                                                                           void *payload)
{
    auto *gm = static_cast<core::device::manager::GraphicsContainer *>(graphicsManagers);

    auto *p = static_cast<PipelineBuildCompletePayload *>(payload);

    auto handle = Handle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                             common::special_types::PipelineTypeName),
                         .id = p->handleID};

    std::cout << "Pipeline at [" << p->handleID << "] is ready" << std::endl;

    gm->pipelineManager->get(handle)->request.pipeline = std::move(*p->pipeline);
}

star::job::complete_tasks::CompleteTask star::job::complete_tasks::task_factory::CreateBuildPipelineComplete(
    uint32_t handleID, std::unique_ptr<StarPipeline> pipeline)
{
    return CompleteTask::Builder<PipelineBuildCompletePayload>()
        .setPayload(PipelineBuildCompletePayload{.handleID = std::move(handleID), .pipeline = std::move(pipeline)})
        .setEngineExecuteFunction(&ExecuteBuildPipelineComplete)
        .build();
}

#pragma endregion BuildPipeline

#pragma region CompileShaders

#pragma endregion CompileShaders