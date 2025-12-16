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

constexpr std::string_view PipelineCreateEventTypeName = "star::event::pipeline";

class Pipeline : public TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50>
{
  public:
    Pipeline()
        : TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50>(common::special_types::PipelineTypeName,
                                                                          PipelineCreateEventTypeName)
    {
    }
    virtual ~Pipeline() = default;

    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;

    void init(device::StarDevice *device, common::EventBus &bus, job::TaskManager &taskSystem) override;

    virtual void cleanupRender() override;

  protected:
    absl::flat_hash_map<uint16_t, Handle> m_subscriberShaderBuildInfo;

    PipelineRecord createRecord(PipelineRequest &&request) const override
    {
        return PipelineRecord(std::move(request));
    }

  private:
    void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                    common::EventBus &eventBus, PipelineRecord *storedRecord) override;
};
} // namespace star::core::device::manager