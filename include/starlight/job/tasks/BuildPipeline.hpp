#pragma once

#include "StarPipeline.hpp"
#include "job/tasks/Task.hpp"
#include "starlight/wrappers/graphics/StarPipelineConstants.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::job::tasks::build_pipeline
{

constexpr std::string_view BuildPipelineTaskName = "star::job::tasks::build_pipeline";

struct PipelineData
{
    star::StarPipeline::RenderResourceDependencies deps;
    star::PipelineProvider provider;
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