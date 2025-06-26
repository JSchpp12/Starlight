#pragma once

#include "Handle.hpp"
#include "StarBuffer.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDevice.hpp"

#include "internals/CommandBufferContainer.hpp"

#include <functional>
#include <stack>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace star
{
class ManagerCommandBuffer
{
  public:
    struct CommandBufferRequest
    {
        // when
        // type
        // callback function to record the buffer
        // is it dependent on another buffer to finish first?
        std::function<void(vk::CommandBuffer &, const int &)> recordBufferCallback;
        std::function<void(star::Handle)> promiseBufferHandleCallback;
        Command_Buffer_Order order;
        int orderIndex;
        star::Queue_Type type;
        vk::PipelineStageFlags waitStage;
        bool willBeSubmittedEachFrame;
        bool recordOnce;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback;
        std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
            overrideBufferSubmissionCallback;

        CommandBufferRequest(
            std::function<void(vk::CommandBuffer &, const int &)> recordBufferCallback,
            std::function<void(star::Handle)> promiseBufferHandleCallback, const Command_Buffer_Order &order,
            const int orderIndex, const star::Queue_Type &type, const vk::PipelineStageFlags &waitStage,
            const bool &willBeSubmittedEachFrame, const bool &recordOnce,
            std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback,
            std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
                overrideBufferSubmissionCallback)
            : recordBufferCallback(recordBufferCallback), promiseBufferHandleCallback(promiseBufferHandleCallback),
              order(order), orderIndex(orderIndex), type(type), waitStage(waitStage),
              willBeSubmittedEachFrame(willBeSubmittedEachFrame), recordOnce(recordOnce),
              beforeBufferSubmissionCallback(beforeBufferSubmissionCallback),
              overrideBufferSubmissionCallback(overrideBufferSubmissionCallback) {};
    };

    ManagerCommandBuffer(StarDevice &device, const int &numFramesInFlight);

    ~ManagerCommandBuffer();

    static void request(std::function<CommandBufferRequest(void)> request);

    static void callPreRecordFunctions(const uint8_t &frameInFlightIndex);

    static void submitDynamicBuffer(Handle bufferHandle);

    /// @brief Process and submit all command buffers
    /// @param frameIndexToBeDrawn
    /// @return semaphore signaling completion of submission
    vk::Semaphore update(const int &frameIndexToBeDrawn);

  private:
    static std::stack<std::function<CommandBufferRequest(void)>> newCommandBufferRequests;

    static std::stack<Handle> dynamicBuffersToSubmit;

    StarDevice &device;
    const int numFramesInFlight = 0;
    CommandBufferContainer buffers;
    std::unique_ptr<star::Handle> mainGraphicsBufferHandle = std::unique_ptr<star::Handle>();

    void handleNewRequests();

    vk::Semaphore submitCommandBuffers(const uint32_t &swapChainIndex);

    void handleDynamicBufferRequests();
};
} // namespace star