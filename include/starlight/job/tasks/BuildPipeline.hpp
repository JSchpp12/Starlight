#pragma once

#include "StarPipeline.hpp"
#include "job/tasks/Task.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::job::tasks::build_pipeline
{

constexpr std::string_view BuildPipelineTaskName = "star::job::tasks::build_pipeline";

struct PipelineBuildPayload
{
    vk::Device device;
    uint32_t handleID;
    std::unique_ptr<star::StarPipeline::RenderResourceDependencies> deps = nullptr;
    std::unique_ptr<star::StarPipeline> pipeline = nullptr;
};

using BuildPipelineTask = star::job::tasks::Task<sizeof(PipelineBuildPayload), alignof(PipelineBuildPayload)>;

void ExecuteBuildPipeline(void *p);

std::optional<star::job::complete_tasks::CompleteTask> CreateBuildComplete(void *p);

BuildPipelineTask CreateBuildPipeline(vk::Device device, Handle handle,
                                      star::StarPipeline::RenderResourceDependencies buildDeps, StarPipeline pipeline);
} // namespace star::job::tasks::build_pipeline