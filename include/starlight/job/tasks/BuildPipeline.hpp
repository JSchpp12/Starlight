#pragma once

#include "StarPipeline.hpp"
#include "core/device/managers/Manager.hpp"
#include "job/tasks/Task.hpp"
#include "starlight/wrappers/graphics/StarPipelineConstants.hpp"

#include <future>
#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::job::tasks::build_pipeline
{

constexpr std::string_view BuildPipelineTaskName = "star::job::tasks::build_pipeline";

struct PipelineData
{
    star::StarPipeline::RenderResourceDependencies deps;
    star::PipelineProvider provider;
    vk::Device device;
    uint32_t handleID;
    /// @brief Producer side of the record-level built-pipeline future.
    std::promise<star::core::device::manager::PendingResult<star::StarPipeline>> pipelinePromise;
};

struct PipelineBuildPayload
{
    std::unique_ptr<PipelineData> data;
};

using BuildPipelineTask = star::job::tasks::Task<sizeof(PipelineBuildPayload), alignof(PipelineBuildPayload)>;

void ExecuteBuildPipeline(void *p);

std::optional<star::job::complete_tasks::CompleteTask> CreateBuildComplete(void *p);

BuildPipelineTask CreateBuildPipeline(
    vk::Device device, Handle handle, star::StarPipeline::RenderResourceDependencies buildDeps,
    PipelineProvider provider,
    std::promise<star::core::device::manager::PendingResult<star::StarPipeline>> pipelinePromise);
} // namespace star::job::tasks::build_pipeline