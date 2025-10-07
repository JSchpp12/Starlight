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
        : pipeline(std::move(pipeline)), resolution(std::move(resolution)), renderingInfo(std::move(renderingInfo))
    {
    }
    ~PipelineRequest() = default;
    PipelineRequest(const PipelineRequest &) = delete;
    PipelineRequest &operator=(const PipelineRequest &) = delete;
    PipelineRequest(PipelineRequest &&other)
        : pipeline(std::move(other.pipeline)), resolution(std::move(other.resolution)),
          renderingInfo(std::move(other.renderingInfo))

    {
    }
    PipelineRequest &operator=(PipelineRequest &&other)
    {
        if (this != &other)
        {
            pipeline = std::move(other.pipeline);
            resolution = other.resolution;
            renderingInfo = other.renderingInfo;
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
            numCompiled = other.numCompiled;
        }
        return *this;
    };

    bool isReady() const
    {
        return request.pipeline.isRenderReady();
    }

    void cleanupRender(core::device::StarDevice &device)
    {
        request.pipeline.cleanupRender(device);
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

    PipelineRecord createRecord(device::StarDevice &device, PipelineRequest &&request) const override
    {
        return PipelineRecord(std::move(request));
    }

  private:
    void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                    system::EventBus &eventBus, PipelineRecord *storedRecord) override;
};
} // namespace star::core::device::manager