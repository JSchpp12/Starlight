#pragma once

#include "StarPipeline.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"
#include "device/managers/TaskCreatedResourceManager.hpp"

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

    StarPipeline pipeline = StarPipeline();
    vk::Extent2D resolution = vk::Extent2D();
    star::core::renderer::RenderingTargetInfo renderingInfo = star::core::renderer::RenderingTargetInfo();
};

struct PipelineRecord
{
    PipelineRecord() = default;
    PipelineRecord(PipelineRequest pipeline) : request(std::move(pipeline)){};

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

class Pipeline : public TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50>
{
  public:
    Pipeline()
        : TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50>(common::special_types::PipelineTypeName(),
                                                                          "pipline_event_callback")
    {
    }
    virtual ~Pipeline() = default;

    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;

    void init(std::shared_ptr<device::StarDevice> device, core::device::system::EventBus &bus) override;

    virtual void cleanup(core::device::system::EventBus &bus) override;

  protected:
    std::unordered_map<Handle, Handle, star::HandleHash> m_subscriberShaderBuildInfo;
    uint16_t m_shaderBuiltCallbackEventType;

    PipelineRecord createRecord(device::StarDevice &device, PipelineRequest &&request) const override
    {
        return PipelineRecord(std::move(request));
    }

  private:
    void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                    system::EventBus &eventBus, PipelineRecord *storedRecord) override;
};
} // namespace star::core::device::manager