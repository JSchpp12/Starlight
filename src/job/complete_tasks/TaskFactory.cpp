#include "complete_tasks/TaskFactory.hpp"

#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"
#include "job/tasks/task_factory/TaskFactory.hpp"

#include "core/device/StarDevice.hpp"

#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/system/EventBus.hpp"
#include "core/device/system/event/ShaderCompiled.hpp"

#pragma region BuildPipeline

void star::job::complete_tasks::task_factory::ExecuteBuildPipelineComplete(void *device, void *taskSystem,
                                                                           void *eventBus, void *graphicsManagers,
                                                                           void *payload)
{
    auto *gm = static_cast<core::device::manager::GraphicsContainer *>(graphicsManagers);

    auto *p = static_cast<PipelineBuildCompletePayload *>(payload);

    auto handle = Handle{
        .id = p->handleID,
        .type = Handle_Type::pipeline
    };

    std::cout << "Pipeline at [" << p->handleID << "] is ready" << std::endl;

    gm->pipelineManager.get(handle)->request.pipeline = std::move(*p->pipeline);
}

star::job::complete_tasks::CompleteTask<> star::job::complete_tasks::task_factory::CreateBuildPipelineComplete(
    uint32_t handleID, std::unique_ptr<StarPipeline> pipeline)
{
    return CompleteTask<>::Builder<PipelineBuildCompletePayload>()
        .setPayload(PipelineBuildCompletePayload{.handleID = std::move(handleID), .pipeline = std::move(pipeline)})
        .setEngineExecuteFunction(&ExecuteBuildPipelineComplete)
        .build();
}

#pragma endregion BuildPipeline

#pragma region CompileShaders
void star::job::complete_tasks::task_factory::ExecuteShaderCompileComplete(void *device, void *taskSystem,
                                                                           void *eventBus, void *graphicsManagers,
                                                                           void *payload)
{
    auto *d = static_cast<core::device::StarDevice *>(device);
    auto *gm = static_cast<star::core::device::manager::GraphicsContainer *>(graphicsManagers);
    auto *eb = static_cast<star::core::device::system::EventBus *>(eventBus);
    auto *p = static_cast<CompileCompletePayload *>(payload);

    Handle shader = Handle{
        .id = p->handleID,
        .type = Handle_Type::shader
    };

    assert(p->compiledShaderCode != nullptr && "Compiled shader data not properly set");

    std::cout << "Marking shader at index [" << p->handleID << "] as ready" << std::endl;
    gm->shaderManager.get(shader)->compiledShader = std::move(p->compiledShaderCode);
    eb->emit<core::device::system::event::ShaderCompiled>(core::device::system::event::ShaderCompiled{shader});

    ProcessPipelinesWhichAreNowReadyForBuild(device, taskSystem, graphicsManagers);
}

void star::job::complete_tasks::task_factory::ProcessPipelinesWhichAreNowReadyForBuild(void *device, void *taskSystem,
                                                                                       void *graphicsManagers)
{
    assert(graphicsManagers != nullptr && "Managers pointer is null");

    auto *d = static_cast<core::device::StarDevice *>(device);
    auto *gm = static_cast<core::device::manager::GraphicsContainer *>(graphicsManagers);
    auto *ts = static_cast<job::TaskManager *>(taskSystem);

    for (size_t i = 0; i < gm->pipelineManager.getRecords().getData().size(); i++)
    {
        auto &record = gm->pipelineManager.getRecords().getData()[i];
        if (!record.isReady() && record.numCompiled != 0 &&
            record.numCompiled == record.request.pipeline.getShaders().size())
        {
            uint32_t recordHandle = 0;
            if (!star::CastHelpers::SafeCast<size_t, uint32_t>(i, recordHandle))
            {
                throw std::runtime_error("Unknown error. Failed to process record handle");
            }

            Handle handle = Handle{
                .id = recordHandle,
                .type = Handle_Type::pipeline
            };

            std::vector<std::pair<StarShader, std::unique_ptr<std::vector<uint32_t>>>> compiledShaders;
            for (auto &shader : record.request.pipeline.getShaders())
            {
                compiledShaders.push_back(std::make_pair<StarShader, std::unique_ptr<std::vector<uint32_t>>>(
                    StarShader(gm->shaderManager.get(shader)->request.shader),
                    std::move(gm->shaderManager.get(shader)->compiledShader)));
            }

            star::StarPipeline::RenderResourceDependencies deps{.compiledShaders = std::move(compiledShaders),
                                                                .renderingTargetInfo = record.request.renderingInfo,
                                                                .swapChainExtent = record.request.resolution};

            ts->submitTask(tasks::task_factory::CreateBuildPipeline(d->getVulkanDevice(), handle, std::move(deps),
                                                                    std::move(record.request.pipeline)));
        }
    }
}
star::job::complete_tasks::CompleteTask<> star::job::complete_tasks::task_factory::CreateShaderCompileComplete(
    uint32_t handleID, std::unique_ptr<StarShader> finalizedShaderObject,
    std::unique_ptr<std::vector<uint32_t>> finalizedCompiledShader)
{
    return complete_tasks::CompleteTask<>::Builder<CompileCompletePayload>()
        .setPayload(CompileCompletePayload{.handleID = std::move(handleID),
                                           .finalizedShaderObject = std::move(finalizedShaderObject),
                                           .compiledShaderCode = std::move(finalizedCompiledShader)})
        .setEngineExecuteFunction(&ExecuteShaderCompileComplete)
        // .setMovePayloadFunction(&MoveShaderCompletePaylod)
        // .setDestroyFunction(&DestroyShaderCompletePayload)
        .build();
}

#pragma endregion CompileShaders