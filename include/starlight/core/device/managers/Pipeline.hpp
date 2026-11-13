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
    PipelineRequest(PipelineProvider provider) : provider(std::move(provider))
    {
    }
    PipelineRequest(PipelineProvider provider, vk::Extent2D resolution, renderer::RenderingTargetInfo renderingInfo)
        : provider(std::move(provider)), resolution(std::move(resolution)), renderingInfo(std::move(renderingInfo))
    {
    }

    PipelineProvider provider = PipelineProvider();
    vk::Extent2D resolution = vk::Extent2D();
    star::core::renderer::RenderingTargetInfo renderingInfo = star::core::renderer::RenderingTargetInfo();
};

struct PipelineRecord
{
    PipelineRecord() = default;
    PipelineRecord(PipelineRequest request) : request(std::move(request)) {};

    /// @brief True once the built pipeline is available (resolves a ready record-level future).
    bool isReady() const
    {
        if (builtPipeline.isRenderReady())
            return true;

        if (!pending.has_value() || !pending->isReady())
            return false;

        resolve();
        return builtPipeline.isRenderReady();
    }

    /// @brief Consume a ready build result into builtPipeline. Never blocks; no-op while pending.
    void resolve() const;

    /// @brief Block until the build worker publishes, then resolve.
    void waitForReady() const;

    void cleanupRender(core::device::StarDevice &device);

    PipelineRequest request = PipelineRequest();
    mutable StarPipeline builtPipeline;
    /// @brief Record-level future for the built pipeline; created at submit, resolved by the build worker.
    mutable std::optional<PendingResource<StarPipeline>> pending;
    /// @brief Producer side of pending; moved into the build task when the build is queued.
    std::optional<std::promise<PendingResult<StarPipeline>>> buildPromise;
    bool buildQueued = false;
};

constexpr std::string_view PipelineCreateEventTypeName = "stPipE";

/// @brief Compile-time name of the pipeline resource type, used for handle registration and failure logging.
struct PipelineResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return common::special_types::PipelineTypeName;
    }
};

class Pipeline : public TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50, PipelineResourceTypeName>
{
  public:
    Pipeline()
        : TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50, PipelineResourceTypeName>(
              PipelineCreateEventTypeName)
    {
    }
    virtual ~Pipeline() = default;
    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;

    void init(device::StarDevice *device, common::EventBus &eventBus, job::TaskManager &taskSystem) override;

  protected:
    PipelineRecord createRecord(PipelineRequest &&request) const override
    {
        PipelineRecord record(std::move(request));

        auto promise = std::promise<PendingResult<StarPipeline>>{};
        record.pending.emplace(PendingResource<StarPipeline>{promise.get_future()});
        record.buildPromise.emplace(std::move(promise));

        return record;
    }

  private:
    void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                    common::EventBus &eventBus, PipelineRecord *storedRecord) override;
};
} // namespace star::core::device::manager