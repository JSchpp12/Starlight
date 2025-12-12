#include "job/complete_tasks/CompileShader.hpp"

#include "core/device/StarDevice.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/system/event/ShaderCompiled.hpp"
#include "job/tasks/TaskFactory.hpp"
#include <starlight/common/EventBus.hpp>

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>

namespace star::job::complete_tasks::compile_shader
{
void ExecuteShaderCompileComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers, void *payload)
{
    // auto *d = static_cast<core::device::StarDevice *>(device);
    auto *gm = static_cast<star::core::device::manager::GraphicsContainer *>(graphicsManagers);
    auto *eb = static_cast<star::common::EventBus *>(eventBus);
    auto *p = static_cast<CompileCompletePayload *>(payload);

    Handle shader = Handle{
        .type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::ShaderTypeName),
        .id = p->handleID};
    assert(p->compiledShaderCode != nullptr && "Compiled shader data not properly set");

    std::cout << "Marking shader at index [" << p->handleID << "] as ready" << std::endl;
    gm->shaderManager->get(shader)->setCompiledShader(std::move(p->compiledShaderCode));
    eb->emit<core::device::system::event::ShaderCompiled>(core::device::system::event::ShaderCompiled{shader});

    ProcessPipelinesWhichAreNowReadyForBuild(device, taskSystem, graphicsManagers);
}

void ProcessPipelinesWhichAreNowReadyForBuild(void *device, void *taskSystem, void *graphicsManagers)
{
    assert(graphicsManagers != nullptr && "Managers pointer is null");

    auto *d = static_cast<core::device::StarDevice *>(device);
    auto *gm = static_cast<core::device::manager::GraphicsContainer *>(graphicsManagers);
    auto *ts = static_cast<job::TaskManager *>(taskSystem);

    for (size_t i = 0; i < gm->pipelineManager->getRecords().getData().size(); i++)
    {
        auto &record = gm->pipelineManager->getRecords().getData()[i];
        if (!record.isReady() && record.numCompiled != 0 &&
            record.numCompiled == record.request.pipeline.getShaders().size())
        {
            uint32_t recordHandle = 0;
            if (!star::common::helper::SafeCast<size_t, uint32_t>(i, recordHandle))
            {
                throw std::runtime_error("Unknown error. Failed to process record handle");
            }

            Handle handle = Handle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                       common::special_types::PipelineTypeName),
                                   .id = recordHandle};

            std::vector<std::pair<StarShader, std::shared_ptr<std::vector<uint32_t>>>> compiledShaders;
            for (auto &shader : record.request.pipeline.getShaders())
            {
                compiledShaders.push_back(std::make_pair<StarShader, std::shared_ptr<std::vector<uint32_t>>>(
                    StarShader(gm->shaderManager->get(shader)->request.shader),
                    gm->shaderManager->get(shader)->giveMeCompiledShader()));
            }

            star::StarPipeline::RenderResourceDependencies deps{.compiledShaders = std::move(compiledShaders),
                                                                .renderingTargetInfo = record.request.renderingInfo,
                                                                .swapChainExtent = record.request.resolution};

            ts->submitTask(tasks::build_pipeline::CreateBuildPipeline(d->getVulkanDevice(), handle, std::move(deps),
                                                                      std::move(record.request.pipeline)),
                           tasks::build_pipeline::BuildPipelineTaskName);
        }
    }
}

star::job::complete_tasks::CompleteTask CreateShaderCompileComplete(
    uint32_t handleID, std::unique_ptr<StarShader> finalizedShaderObject,
    std::shared_ptr<std::vector<uint32_t>> finalizedCompiledShader)
{
    return complete_tasks::CompleteTask::Builder<CompileCompletePayload>()
        .setPayload(CompileCompletePayload{.handleID = std::move(handleID),
                                           .finalizedShaderObject = std::move(finalizedShaderObject),
                                           .compiledShaderCode = std::move(finalizedCompiledShader)})
        .setEngineExecuteFunction(&ExecuteShaderCompileComplete)
        // .setMovePayloadFunction(&MoveShaderCompletePaylod)
        // .setDestroyFunction(&DestroyShaderCompletePayload)
        .build();
}
} // namespace star::job::complete_tasks::compile_shader
