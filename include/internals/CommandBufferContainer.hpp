#pragma once

#include "Handle.hpp"
#include "StarCommandBuffer.hpp"
#include "device/StarDevice.hpp"

#include <vulkan/vulkan.hpp>

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
        std::function<void(vk::CommandBuffer &, const uint8_t &, const uint64_t &)> recordBufferCallback;
        std::unique_ptr<StarCommandBuffer> commandBuffer;
        Queue_Type type;
        bool recordOnce;
        vk::PipelineStageFlags waitStage;
        Command_Buffer_Order order;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback;
        std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const uint8_t &, std::vector<vk::Semaphore>*, std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags>)>>
            overrideBufferSubmissionCallback;
        std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>> oneTimeWaitSemaphores;

        CompleteRequest(
            std::function<void(vk::CommandBuffer &, const uint8_t &, const uint64_t &)> recordBufferCallback,
            std::unique_ptr<StarCommandBuffer> commandBuffer, const Queue_Type &type, const bool &recordOnce,
            const vk::PipelineStageFlags &waitStage, const Command_Buffer_Order &order, 
            std::optional<std::function<void(const int &)>> beforeSubmissionCallback =
                std::optional<std::function<void(const int &)>>(),
            std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const uint8_t &, std::vector<vk::Semaphore>*, std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags>)>>
                overrideBufferSubmissionCallback = std::nullopt)
            : recordBufferCallback(recordBufferCallback), commandBuffer(std::move(commandBuffer)), type(type),
              recordOnce(recordOnce), waitStage(waitStage), order(order),
              beforeBufferSubmissionCallback(beforeSubmissionCallback),
              overrideBufferSubmissionCallback(overrideBufferSubmissionCallback) {};

        void giveMeOneTimeSemaphoreWaitInfo(std::vector<vk::Semaphore> &semaphores, std::vector<vk::PipelineStageFlags> &waitPoints){
            for (const auto &waitInfo : oneTimeWaitSemaphores){
                semaphores.push_back(waitInfo.first);
                waitPoints.push_back(waitInfo.second);
            }

            oneTimeWaitSemaphores.clear(); 
        }

        vk::Semaphore submitCommandBuffer(core::device::StarDevice &device, const uint8_t &frameInFlightIndex, std::vector<vk::Semaphore> *beforeSemaphores = nullptr){
            auto waits = std::vector<vk::Semaphore>(); 
            auto waitPoints = std::vector<vk::PipelineStageFlags>(); 
            giveMeOneTimeSemaphoreWaitInfo(waits, waitPoints);  
            
            if (overrideBufferSubmissionCallback.has_value()){
                return overrideBufferSubmissionCallback.value()(*commandBuffer, frameInFlightIndex, beforeSemaphores, std::move(waits), std::move(waitPoints)); 
            }

            commandBuffer->submit(frameInFlightIndex, device.getDefaultQueue(commandBuffer->getType()).getVulkanQueue()); 
            return commandBuffer->getCompleteSemaphores().at(frameInFlightIndex); 

        }
    };

    CommandBufferContainer(const int &numImagesInFlight, core::device::StarDevice &device);

    ~CommandBufferContainer() = default;

    void cleanup(core::device::StarDevice &device);

    std::vector<vk::Semaphore> submitGroupWhenReady(core::device::StarDevice &device, const star::Command_Buffer_Order &order,
                                                    const uint8_t &frameInFlightIndex, const uint64_t &currentFrameIndex,
                                                    std::vector<vk::Semaphore> *waitSemaphores = nullptr);

    star::Handle add(std::unique_ptr<CompleteRequest> newRequest, const bool &willBeSubmittedEachFrame,
                     const star::Queue_Type &type, const star::Command_Buffer_Order &order,
                     const star::Command_Buffer_Order_Index &subOrder);

    bool shouldSubmitThisBuffer(const size_t &bufferIndex);

    void resetThisBufferStatus(const size_t &bufferIndex);

    void setToSubmitThisBuffer(const size_t &bufferIndex);

    CompleteRequest &get(const star::Handle &bufferHandle);

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

        GenericBufferGroupInfo(const int &numFramesInFlight, core::device::StarDevice &device)
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
                    vk::Fence newFence = device.getVulkanDevice().createFence(fenceInfo);
                    this->fences[static_cast<star::Queue_Type>(i)].push_back(newFence);

                    this->semaphores[static_cast<star::Queue_Type>(i)].push_back(
                        device.getVulkanDevice().createSemaphore(semaphoreInfo));
                }
            }
        };

        ~GenericBufferGroupInfo()
        {
            assert(this->semaphores.empty() && "Not all semaphores have been destroyed");
        };

        void cleanup(core::device::StarDevice &device)
        {
            {
                std::vector<star::Queue_Type> types;
                for (auto &typeSemaphore : this->semaphores)
                {
                    for (auto &semaphore : typeSemaphore.second)
                    {
                        device.getVulkanDevice().destroySemaphore(semaphore);
                    }
                    types.push_back(typeSemaphore.first);
                }

                for (const auto &type : types)
                    this->semaphores.erase(type);
            }
            {
                std::vector<star::Queue_Type> types;
                for (auto &typeFence : this->fences)
                {
                    for (auto &fence : typeFence.second)
                    {
                        device.getVulkanDevice().destroyFence(fence);
                    }
                }

                for (const auto &type : types)
                    this->fences.erase(type);
            }
        }

      private:
    };

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

    void waitUntilOrderGroupReady(core::device::StarDevice &device, const int &frameIndex, const star::Command_Buffer_Order &order,
                                  const star::Queue_Type &type);

    std::vector<std::reference_wrapper<CompleteRequest>> getAllBuffersOfTypeAndOrderReadyToSubmit(
        const star::Command_Buffer_Order &order, const star::Queue_Type &type, bool triggerReset = false);

    void updateSemaphores();
};
} // namespace star