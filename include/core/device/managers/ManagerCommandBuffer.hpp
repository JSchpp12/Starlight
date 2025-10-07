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

namespace star::core::device::managers
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
        std::function<void(vk::CommandBuffer &, const int &)> recordBufferCallback;
        std::function<std::set<Handle>(const uint8_t &)> getSemaphoresToWaitOnForSubmission; 
        Command_Buffer_Order order;
        int orderIndex;
        star::Queue_Type type;
        vk::PipelineStageFlags waitStage = vk::PipelineStageFlags();
        bool willBeSubmittedEachFrame = false;
        bool recordOnce = false;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback = std::nullopt;
        std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
            overrideBufferSubmissionCallback = std::nullopt;
    };

    ManagerCommandBuffer(StarDevice &device, const uint8_t &numFramesInFlight);

    ~ManagerCommandBuffer() = default;

    void cleanup(StarDevice &device);

    Handle submit(StarDevice &device, Request requestFunction);

    void callPreRecordFunctions(const uint8_t &frameInFlightIndex);

    void submitDynamicBuffer(Handle bufferHandle);

    /// @brief Process and submit all command buffers
    /// @param frameIndexToBeDrawn
    /// @return semaphore signaling completion of submission
    vk::Semaphore update(StarDevice &device, const int &frameIndexToBeDrawn);

  private:
    static std::stack<Handle> dynamicBuffersToSubmit;

    uint8_t numFramesInFlight = 0;
    CommandBufferContainer buffers;
    std::unique_ptr<star::Handle> mainGraphicsBufferHandle = std::unique_ptr<star::Handle>();

    vk::Semaphore submitCommandBuffers(StarDevice &device, const uint8_t &swapChainIndex);

    void handleDynamicBufferRequests();
};
} // namespace star