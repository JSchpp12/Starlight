#pragma once

#include "Handle.hpp"
#include "StarCommandBuffer.hpp"
#include "StarDevice.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace star
{
class CommandBufferContainer
{
  public:
    struct CompleteRequest
    {
        std::function<void(vk::CommandBuffer &, const int &)> recordBufferCallback;
        std::unique_ptr<StarCommandBuffer> commandBuffer;
        Queue_Type type;
        bool recordOnce;
        vk::PipelineStageFlags waitStage;
        Command_Buffer_Order order;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback;
        std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
            overrideBufferSubmissionCallback;

        CompleteRequest(
            std::function<void(vk::CommandBuffer &, const int &)> recordBufferCallback,
            std::unique_ptr<StarCommandBuffer> commandBuffer, const Queue_Type &type, const bool &recordOnce,
            const vk::PipelineStageFlags &waitStage, const Command_Buffer_Order &order,
            std::optional<std::function<void(const int &)>> beforeSubmissionCallback =
                std::optional<std::function<void(const int &)>>(),
            std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
                overrideBufferSubmissionCallback = std::nullopt)
            : recordBufferCallback(recordBufferCallback), commandBuffer(std::move(commandBuffer)), order(order),
              waitStage(waitStage), beforeBufferSubmissionCallback(beforeSubmissionCallback),
              overrideBufferSubmissionCallback(overrideBufferSubmissionCallback), type(type), recordOnce(recordOnce) {};
    };

    CommandBufferContainer(StarDevice &device, const int &numImagesInFlight);

    ~CommandBufferContainer() = default;

    std::vector<vk::Semaphore> submitGroupWhenReady(const star::Command_Buffer_Order &order,
                                                    const int &frameInFlightIndex,
                                                    std::vector<vk::Semaphore> *waitSemaphores = nullptr);

    star::Handle add(std::unique_ptr<CompleteRequest> newRequest, const bool &willBeSubmittedEachFrame,
                     const star::Queue_Type &type, const star::Command_Buffer_Order &order,
                     const star::Command_Buffer_Order_Index &subOrder);

    bool shouldSubmitThisBuffer(const size_t &bufferIndex);

    void resetThisBufferStatus(const size_t &bufferIndex);

    void setToSubmitThisBuffer(const size_t &bufferIndex);

    CompleteRequest &getBuffer(const star::Handle &bufferHandle);

    size_t size()
    {
        return this->allBuffers.size();
    }

  private:
    struct GenericBufferGroupInfo
    {
        std::unordered_map<star::Queue_Type, std::vector<size_t>> bufferOrderGroupsIndices =
            std::unordered_map<star::Queue_Type, std::vector<size_t>>();
        std::unordered_map<star::Queue_Type, std::vector<vk::Semaphore>> semaphores =
            std::unordered_map<star::Queue_Type, std::vector<vk::Semaphore>>();
        std::unordered_map<star::Queue_Type, std::vector<vk::Fence>> fences =
            std::unordered_map<star::Queue_Type, std::vector<vk::Fence>>();

        GenericBufferGroupInfo(StarDevice &device, const int &numFramesInFlight) : device(device)
        {
            vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
            semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

            vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo();
            fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            for (int j = 0; j < numFramesInFlight; j++)
            {
                for (int i = star::Queue_Type::Tgraphics; i != star::Queue_Type::Tcompute; i++)
                {
                    vk::Fence newFence = device.getDevice().createFence(fenceInfo);
                    this->fences[static_cast<star::Queue_Type>(i)].push_back(newFence);

                    this->semaphores[static_cast<star::Queue_Type>(i)].push_back(
                        device.getDevice().createSemaphore(semaphoreInfo));
                }
            }
        };

        ~GenericBufferGroupInfo()
        {
            for (auto &typeSemaphore : this->semaphores)
                for (auto &semaphore : typeSemaphore.second)
                    this->device.getDevice().destroySemaphore(semaphore);

            for (auto &typeFence : this->fences)
                for (auto &fence : typeFence.second)
                    this->device.getDevice().destroyFence(fence);
        };

      private:
        StarDevice &device;
    };

    StarDevice &device;
    std::vector<std::unique_ptr<CompleteRequest>> allBuffers = std::vector<std::unique_ptr<CompleteRequest>>();
    std::unordered_map<star::Command_Buffer_Order, std::unique_ptr<GenericBufferGroupInfo>> bufferGroupsWithNoSubOrder =
        std::unordered_map<star::Command_Buffer_Order, std::unique_ptr<GenericBufferGroupInfo>>();
    std::unordered_map<star::Command_Buffer_Order, std::vector<CompleteRequest *>> bufferGroupsWithSubOrders =
        std::unordered_map<star::Command_Buffer_Order, std::vector<CompleteRequest *>>();

    // 0 - no
    // 1 - dynamic submit
    // 2 - standard always submit
    std::vector<unsigned char> bufferSubmissionStatus = std::vector<unsigned char>();

    // Indicates if all semaphores are updated with the proper order of execution
    bool subOrderSemaphoresUpToDate = false;

    void waitUntilOrderGroupReady(const int &frameIndex, const star::Command_Buffer_Order &order,
                                  const star::Queue_Type &type);

    std::vector<std::reference_wrapper<CompleteRequest>> getAllBuffersOfTypeAndOrderReadyToSubmit(
        const star::Command_Buffer_Order &order, const star::Queue_Type &type, bool triggerReset = false);

    void updateSemaphores();
};
} // namespace star