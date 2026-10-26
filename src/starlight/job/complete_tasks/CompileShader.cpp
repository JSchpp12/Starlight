#include "job/complete_tasks/CompileShader.hpp"

#include "core/device/StarDevice.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/system/event/ShaderCompiled.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "logging/LoggingFactory.hpp"
#include <star_common/EventBus.hpp>

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <vulkan/vulkan.hpp>

#include <exception>
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

    star::StarShader compiledShader;
    try
    {
        compiledShader = p->shaderFuture.get();
    }
    catch (const std::exception &e)
    {
        star::core::logging::error("Shader compilation failed for handle [", p->handleID, "]: ", e.what());
        return;
    }

    std::cout << "Marking shader at index [" << p->handleID << "] as ready" << std::endl;
    gm->shaderManager->get(shader)->setCompiledShader(std::move(compiledShader));
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
        auto &record = gm->pipelineManager->getRecords().getData()[i];
        if (!record.isReady() && record.numCompiled != 0 &&
            record.numCompiled == record.request.provider.getShaders().size())
        {
            uint32_t recordHandle = 0;
            if (!star::common::casts::SafeCast<size_t, uint32_t>(i, recordHandle))
            {
                throw std::runtime_error("Unknown error. Failed to process record handle");
            }

            Handle handle = Handle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                       common::special_types::PipelineTypeName),
                                   .id = recordHandle};

            std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
            const auto &shaderHandles = record.request.provider.getShaders();
            shaderStages.reserve(shaderHandles.size());

            bool allShadersReady = true;
            for (const auto &shaderHandle : shaderHandles)
            {
                auto *shaderRecord = gm->shaderManager->get(shaderHandle);
                if (shaderRecord == nullptr || !shaderRecord->isReady())
                {
                    allShadersReady = false;
                    break;
                }

                shaderStages.push_back(shaderRecord->request.shader.getStageCreateInfo());
            }

            if (!allShadersReady)
            {
                continue;
            }

            star::StarPipeline::RenderResourceDependencies deps{.shaderStages = std::move(shaderStages),
                                                                .renderingTargetInfo = record.request.renderingInfo,
                                                                .swapChainExtent = record.request.resolution};

            ts->submitTask(tasks::build_pipeline::CreateBuildPipeline(d->getVulkanDevice(), handle, std::move(deps),
                                                                      std::move(record.request.provider)),
                           tasks::build_pipeline::BuildPipelineTaskName);
        }
    }
}

star::job::complete_tasks::CompleteTask CreateShaderCompileComplete(uint32_t handleID,
                                                                   std::future<star::StarShader> shaderFuture)
{
    return complete_tasks::CompleteTask::Builder<CompileCompletePayload>()
        .setPayload(CompileCompletePayload{.handleID = handleID, .shaderFuture = std::move(shaderFuture)})
        .setExecuteFunction(&ExecuteShaderCompileComplete)
        .build();
}
} // namespace star::job::complete_tasks::compile_shader
