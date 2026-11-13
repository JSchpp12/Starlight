#include "job/complete_tasks/CompileShader.hpp"

#include "core/Exceptions.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/system/event/ShaderCompiled.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "job/tasks/BuildPipeline.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/helper/CastHelpers.hpp>

#include <vulkan/vulkan.hpp>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace star::job::complete_tasks::compile_shader
{
void ExecuteShaderCompileComplete(void *device, void *taskSystem, void *eventBus, void *graphicsManagers, void *payload)
{
    auto *gm = static_cast<star::core::device::manager::GraphicsContainer *>(graphicsManagers);
    auto *eb = static_cast<star::common::EventBus *>(eventBus);
    auto *p = static_cast<CompileCompletePayload *>(payload);

    Handle shader = Handle{
        .type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::ShaderTypeName),
        .id = p->handleID};

    // Resolves the record-level compile future (the worker already published it).
    gm->shaderManager->waitForReady(shader);

    const auto *record = gm->shaderManager->get(shader);
    if (record == nullptr || !record->isReady())
    {
        // A failed compile must fail loudly: dependent pipeline futures can never resolve, which would otherwise hang
        // the scene-ready wait and the material commit gate.
        STAR_THROWF("Shader compile failed for handle [", p->handleID, "]");
    }

    std::cout << "Marking shader at index [" << p->handleID << "] as ready" << std::endl;
    eb->emit(core::device::system::event::ShaderCompiled{shader});

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
        // Pipeline records are created synchronously; a null slot only exists for an unpublished future.
        auto *recordPtr = gm->pipelineManager->getRecords().getData()[i].get();
        if (recordPtr == nullptr)
            continue;

        auto &record = *recordPtr;
        if (record.isReady() || record.buildQueued)
            continue;

        const auto &shaderHandles = record.request.provider.getShaders();
        if (shaderHandles.empty())
            continue;

        uint32_t recordHandle = 0;
        if (!star::common::casts::SafeCast<size_t, uint32_t>(i, recordHandle))
            throw std::runtime_error("Unknown error. Failed to process record handle");

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        shaderStages.reserve(shaderHandles.size());

        bool allShadersReady = true;
        for (const auto &shaderHandle : shaderHandles)
        {
            // Manager::isReady resolves a ready record future; get() then materializes the compiled shader.
            if (!gm->shaderManager->isReady(shaderHandle))
            {
                allShadersReady = false;
                break;
            }

            const auto *shaderRecord = gm->shaderManager->get(shaderHandle);
            if (shaderRecord == nullptr || !shaderRecord->isReady())
            {
                allShadersReady = false;
                break;
            }

            shaderStages.push_back(shaderRecord->request.shader.getStageCreateInfo());
        }

        if (!allShadersReady)
            continue;

        Handle handle = Handle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                   common::special_types::PipelineTypeName),
                               .id = recordHandle};

        star::StarPipeline::RenderResourceDependencies deps{.shaderStages = std::move(shaderStages),
                                                            .renderingTargetInfo = record.request.renderingInfo,
                                                            .swapChainExtent = record.request.resolution};

        assert(record.buildPromise.has_value() && "Pipeline record is missing its build promise");
        try
        {
            ts->submitTask(tasks::build_pipeline::CreateBuildPipeline(
                               d->getVulkanDevice(), handle, std::move(deps), std::move(record.request.provider),
                               std::move(*record.buildPromise)),
                           tasks::build_pipeline::BuildPipelineTaskName);
            record.buildQueued = true;
        }
        catch (const std::exception &e)
        {
            core::logging::error("Pipeline build submission failed: ", e.what());
            throw;
        }
        record.buildPromise.reset();
    }
}

star::job::complete_tasks::CompleteTask CreateShaderCompileComplete(uint32_t handleID)
{
    return complete_tasks::CompleteTask::Builder<CompileCompletePayload>()
        .setPayload(CompileCompletePayload{.handleID = handleID})
        .setExecuteFunction(&ExecuteShaderCompileComplete)
        .build();
}
} // namespace star::job::complete_tasks::compile_shader
