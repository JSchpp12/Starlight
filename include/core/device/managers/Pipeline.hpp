#pragma once

#include "StarPipeline.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"
#include "device/managers/Manager.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::core::device::manager
{
struct PipelineRequest
{
    PipelineRequest() = default;
    PipelineRequest(StarPipeline pipeline) : pipeline(std::move(pipeline))
    {
    }
    PipelineRequest(StarPipeline pipeline, vk::Extent2D resolution, renderer::RenderingTargetInfo renderingInfo)
        : resolution(std::move(resolution)), renderingInfo(std::move(renderingInfo)), pipeline(std::move(pipeline))
    {
    }
    ~PipelineRequest() = default;
    PipelineRequest(const PipelineRequest &) = delete;
    PipelineRequest &operator=(const PipelineRequest &) = delete;
    PipelineRequest(PipelineRequest &&other)
        : resolution(std::move(other.resolution)), renderingInfo(std::move(other.renderingInfo)),
          pipeline(std::move(other.pipeline))
    {
    }
    PipelineRequest &operator=(PipelineRequest &&other)
    {
        if (this != &other)
        {
            resolution = std::move(other.resolution);
            renderingInfo = std::move(other.renderingInfo);
            pipeline = std::move(other.pipeline);
        }
        return *this;
    }
    StarPipeline pipeline = StarPipeline();
    vk::Extent2D resolution = vk::Extent2D();
    star::core::renderer::RenderingTargetInfo renderingInfo = star::core::renderer::RenderingTargetInfo();
};

struct PipelineRecord
{
    PipelineRecord() = default;
    PipelineRecord(PipelineRequest pipeline) : request(std::move(pipeline)) {};
    ~PipelineRecord() = default;
    PipelineRecord(const PipelineRecord &) = delete;
    PipelineRecord &operator=(const PipelineRecord &) = delete;
    PipelineRecord(PipelineRecord &&other) : request(std::move(other.request)) {};
    PipelineRecord &operator=(PipelineRecord &&other)
    {
        if (this != &other)
        {
            request = std::move(other.request);
        }
        return *this;
    };

    bool isReady() const
    {
        return request.pipeline.isRenderReady();
    }

    PipelineRequest request = PipelineRequest();
    uint8_t numCompiled = 0; 
};

class Pipeline : public Manager<PipelineRecord, PipelineRequest, 50>
{
  public:
  protected:
    Handle_Type getHandleType() const override
    {
        return Handle_Type::pipeline;
    }

  private:
    void submitTask(const Handle &handle, job::TaskManager &taskSystem, system::EventBus &eventBus, PipelineRecord *storedRecord) override;
};
} // namespace star::core::device::manager