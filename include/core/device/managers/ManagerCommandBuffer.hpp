#pragma once

#include "Handle.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCommandBuffer.hpp"
#include "device/StarDevice.hpp"

#include "internals/CommandBufferContainer.hpp"

#include <functional>
#include <set>
#include <stack>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace star::core::device::manager
{
class ManagerCommandBuffer
{
  public:
    struct Request
    {
        // when
        // type
        // callback function to record the buffer
        // is it dependent on another buffer to finish first?
        std::function<void(vk::CommandBuffer &, const uint8_t &, const uint64_t &)> recordBufferCallback;
        Command_Buffer_Order order;
        int orderIndex;
        star::Queue_Type type;
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlags();
        bool willBeSubmittedEachFrame = false;
        bool recordOnce = false;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback = std::nullopt;
        std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const uint8_t &, std::vector<vk::Semaphore>*, std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags>)>>
            overrideBufferSubmissionCallback = std::nullopt;
    };

    ManagerCommandBuffer(StarDevice &device, const uint8_t &numFramesInFlight);

    void cleanup(StarDevice &device);

    Handle submit(StarDevice &device, const uint64_t &currentFrameIndex, Request requestFunction);

    void submitDynamicBuffer(Handle bufferHandle);

    CommandBufferContainer::CompleteRequest &get(const Handle& handle); 

    /// @brief Process and submit all command buffers
    /// @param frameIndexToBeDrawn
    /// @return semaphore signaling completion of submission
    vk::Semaphore update(StarDevice &device, const uint8_t &frameIndexToBeDrawn, const uint64_t &currentFrameIndex);
  private:
    static std::stack<Handle> dynamicBuffersToSubmit;

    uint8_t numFramesInFlight = 0;
    CommandBufferContainer buffers;
    std::unique_ptr<star::Handle> mainGraphicsBufferHandle = std::unique_ptr<star::Handle>();

    vk::Semaphore submitCommandBuffers(StarDevice &device, const uint8_t &swapChainIndex, const uint64_t &currentFrameIndex);

    void handleDynamicBufferRequests();
};
} // namespace star::core::device::manager