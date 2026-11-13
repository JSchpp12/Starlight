#pragma once

#include "StarBuffers/Buffer.hpp"
#include "StarCommandBuffer.hpp"
#include "device/StarDevice.hpp"
#include "internals/CommandBufferContainer.hpp"
#include "starlight/core/device/managers/Queue.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

#include <functional>
#include <stack>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace star::common
{
class EventBus;
}

namespace star::core::device::manager
{
class ManagerCommandBuffer
{
  public:
    using BufferSubmissionOverride = StarCommandBuffer::SubmissionOverride;
    struct Request
    {
        // when
        // type
        // callback function to record the buffer
        // is it dependent on another buffer to finish first?
        std::function<void(StarCommandBuffer &, const common::FrameTracker &, const uint64_t &)> recordBufferCallback;
        Command_Buffer_Order order;
        int orderIndex;
        star::Queue_Type type;
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlags();
        bool willBeSubmittedEachFrame = false;
        bool recordOnce = false;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback = std::nullopt;
        std::optional<BufferSubmissionOverride> overrideBufferSubmissionCallback = std::nullopt;
    };
    struct InUseQueueInfo
    {
        StarCommandPool pool;
        StarQueue *queue = nullptr;
    };

    ManagerCommandBuffer(StarDevice &device, core::device::manager::Queue &queueManager,
                         const uint8_t &numFramesInFlight,
                         const absl::flat_hash_map<star::Queue_Type, Handle> &queuesToUse);

    void init(core::device::manager::Queue &queueManager);

    void cleanup(StarDevice &device);

    Handle submit(StarDevice &device, const uint64_t &currentFrameIndex, Request requestFunction);

    void submitDynamicBuffer(Handle bufferHandle);

    CommandBufferContainer::CompleteRequest &get(const Handle &handle);

    CommandBufferContainer::CompleteRequest &getDefault();

    /// @brief Process and submit all command buffers
    /// @param frameIndexToBeDrawn
    /// @return semaphore signaling completion of submission
    vk::Semaphore update(StarDevice &device, const common::FrameTracker &frameTracker, common::EventBus &eventBus);

    /// @brief Queue frame-wide readiness conditions drained immediately before the finalization phase records.
    void addBeforeFinalizationCPUWait(std::vector<star::ReadinessCondition> waits,
                                      ReadinessFailurePolicy policy = ReadinessFailurePolicy::BlockUntilReady)
    {
        m_beforeFinalizationWaits.add(std::move(waits), std::move(policy));
    }
    const InUseQueueInfo *getInUseInfoForType(const star::Queue_Type &type);

  private:
    static std::stack<Handle> dynamicBuffersToSubmit;
    absl::flat_hash_map<star::Queue_Type, Handle> m_typeToQueueInfo;
    absl::flat_hash_map<Handle, InUseQueueInfo, star::HandleHash> m_inUseQueueInfo;
    CommandBufferContainer buffers;
    uint8_t numFramesInFlight = 0;
    std::unique_ptr<star::Handle> mainGraphicsBufferHandle = std::unique_ptr<star::Handle>();
    absl::flat_hash_map<star::Queue_Type, StarQueue *> m_preparedQueues;
    CPURecordingWaitContainer m_beforeFinalizationWaits;
    vk::Semaphore submitCommandBuffers(StarDevice &device, const common::FrameTracker &swapChainIndex,
                                       const uint64_t &currentFrameIndex,
                                       std::vector<CommandBufferContainer::WaitOutcome> &outOutcomes);

    void handleDynamicBufferRequests();

    InUseQueueInfo *selectQueueForType(const star::Queue_Type &type);
};
} // namespace star::core::device::manager
