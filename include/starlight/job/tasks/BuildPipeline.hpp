#pragma once

#include "StarPipeline.hpp"
#include "job/tasks/Task.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::job::tasks::build_pipeline
{

constexpr std::string_view BuildPipelineTaskName = "star::job::tasks::build_pipeline";

struct PipelineData{
    star::StarPipeline::RenderResourceDependencies deps; 
    /// Build recipe; consumed by ExecuteBuildPipeline.
    star::PipelineProvider provider; 
    /// Built handle; produced by ExecuteBuildPipeline, consumed by the complete task.
    star::StarPipeline built;
    vk::Device device; 
    uint32_t handleID; 
};

struct PipelineBuildPayload
{
    std::unique_ptr<PipelineData> data; 
};

using BuildPipelineTask = star::job::tasks::Task<sizeof(PipelineBuildPayload), alignof(PipelineBuildPayload)>;

void ExecuteBuildPipeline(void *p);

std::optional<star::job::complete_tasks::CompleteTask> CreateBuildComplete(void *p);

BuildPipelineTask CreateBuildPipeline(vk::Device device, Handle handle,
                                      star::StarPipeline::RenderResourceDependencies buildDeps,
                                      PipelineProvider provider);
} // namespace star::job::tasks::build_pipeline