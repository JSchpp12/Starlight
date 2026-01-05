#pragma once

#include "MappedHandleContainer.hpp"
#include "StarCommandBuffer.hpp"
#include "device/StarDevice.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace star
{
class CommandBufferContainer
{
  public:
    struct SemaphoreWaitInfo
    {
        vk::Semaphore semaphore;
        vk::PipelineStageFlags stageFlags;
        std::optional<uint64_t> previousSignaledValue =
            std::nullopt; // if this has a value, this signifies that the semaphore is a TIMELINE semaphore
    };
    class SemaphoreInfo
    {
      public:
        void insert(const Handle &handle, vk::Semaphore semaphore, vk::PipelineStageFlags waitPoints,
                    std::optional<uint64_t> signaledValue = std::nullopt)
        {
            if (m_semaphores.contains(handle))
            {
                m_semaphores[handle].stageFlags |= waitPoints;
            }
            else
            {
                m_semaphores[handle] = SemaphoreWaitInfo{.semaphore = std::move(semaphore),
                                                         .stageFlags = std::move(waitPoints),
                                                         .previousSignaledValue = std::move(signaledValue)};
            }
        }

        void giveMeOneTimeSemaphoreWaitInfo(std::vector<vk::Semaphore> &semaphores,
                                            std::vector<vk::PipelineStageFlags> &waitPoints,
                                            std::vector<std::optional<uint64_t>> &signaledValues)
        {
            for (const auto semaphoreInfo : m_semaphores)
            {
                semaphores.push_back(semaphoreInfo.second.semaphore);
                waitPoints.push_back(semaphoreInfo.second.stageFlags);
                signaledValues.push_back(semaphoreInfo.second.previousSignaledValue);
            }

            m_semaphores.clear();
        }

      private:
        std::unordered_map<Handle, SemaphoreWaitInfo, star::HandleHash> m_semaphores;
        // std::unordered_map<Handle, vk::Semaphore, star::HandleHash> m_semaphores;
        // std::unordered_map<Handle, vk::PipelineStageFlags, star::HandleHash> m_semaphoreWaitFlags;
    };
    struct CompleteRequest
    {
        std::function<void(StarCommandBuffer &, const common::FrameTracker &, const uint64_t &)> recordBufferCallback;
        std::unique_ptr<StarCommandBuffer> commandBuffer;
        Queue_Type type;
        bool recordOnce;
        vk::PipelineStageFlags waitStage;
        Command_Buffer_Order order;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback;
        std::optional<std::function<vk::Semaphore(
            StarCommandBuffer &, const common::FrameTracker &, std::vector<vk::Semaphore> *, std::vector<vk::Semaphore>,
            std::vector<vk::PipelineStageFlags>, std::vector<std::optional<uint64_t>>)>>
            overrideBufferSubmissionCallback;
        SemaphoreInfo oneTimeWaitSemaphoreInfo;

        CompleteRequest(
            std::function<void(StarCommandBuffer &, const common::FrameTracker &, const uint64_t &)>
                recordBufferCallback,
            std::unique_ptr<StarCommandBuffer> commandBuffer, const Queue_Type &type, const bool &recordOnce,
            const vk::PipelineStageFlags &waitStage, const Command_Buffer_Order &order,
            std::optional<std::function<void(const int &)>> beforeSubmissionCallback =
                std::optional<std::function<void(const int &)>>(),
            std::optional<std::function<vk::Semaphore(
                StarCommandBuffer &, const common::FrameTracker &, std::vector<vk::Semaphore> *,
                std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags>, std::vector<std::optional<uint64_t>>)>>
                overrideBufferSubmissionCallback = std::nullopt)
            : recordBufferCallback(recordBufferCallback), commandBuffer(std::move(commandBuffer)), type(type),
              recordOnce(recordOnce), waitStage(waitStage), order(order),
              beforeBufferSubmissionCallback(beforeSubmissionCallback),
              overrideBufferSubmissionCallback(overrideBufferSubmissionCallback) {};

        vk::Semaphore submitCommandBuffer(core::device::StarDevice &device, const common::FrameTracker &frameTracker,
                                          std::vector<vk::Semaphore> *beforeSemaphores = nullptr)
        {
            auto waits = std::vector<vk::Semaphore>();
            auto waitPoints = std::vector<vk::PipelineStageFlags>();
            auto previousSignaledValues = std::vector<std::optional<uint64_t>>();
            oneTimeWaitSemaphoreInfo.giveMeOneTimeSemaphoreWaitInfo(waits, waitPoints, previousSignaledValues);

            if (overrideBufferSubmissionCallback.has_value())
            {
                return overrideBufferSubmissionCallback.value()(*commandBuffer, frameTracker, beforeSemaphores,
                                                                std::move(waits), std::move(waitPoints),
                                                                std::move(previousSignaledValues));
            }
            else
            {
                auto additionalWaits = std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags>>(waits.size());
                for (size_t i = 0; i < waits.size(); i++)
                {
                    additionalWaits[i] = std::make_pair(waits[i], waitPoints[i]);
                }

                if (beforeSemaphores != nullptr)
                {
                    additionalWaits.push_back(std::make_pair(beforeSemaphores->front(), waitStage));
                }

                commandBuffer->submit(frameTracker.getCurrent().getFrameInFlightIndex(),
                                      device.getDefaultQueue(commandBuffer->getType()).getVulkanQueue(),
                                      &additionalWaits);
            }

            return commandBuffer->getCompleteSemaphores().at(frameTracker.getCurrent().getFrameInFlightIndex());
        }
    };

    CommandBufferContainer(const int &numImagesInFlight, core::device::StarDevice &device);

    ~CommandBufferContainer() = default;

    void cleanup(core::device::StarDevice &device);

    std::vector<vk::Semaphore> submitGroupWhenReady(core::device::StarDevice &device,
                                                    const star::Command_Buffer_Order &order,
                                                    const common::FrameTracker &frameTracker,
                                                    const uint64_t &currentFrameIndex,
                                                    std::vector<vk::Semaphore> *waitSemaphores = nullptr);

    star::Handle add(std::shared_ptr<CompleteRequest> newRequest, const bool &willBeSubmittedEachFrame,
                     const star::Queue_Type &type, const star::Command_Buffer_Order &order,
                     const star::Command_Buffer_Order_Index &subOrder);

    bool shouldSubmitThisBuffer(const Handle &bufferHandle);

    void resetThisBufferStatus(const Handle &bufferHandle);

    void setToSubmitThisBuffer(const Handle &bufferHandle);

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

    std::vector<std::shared_ptr<CompleteRequest>> allBuffers = std::vector<std::shared_ptr<CompleteRequest>>();
    std::unordered_map<star::Command_Buffer_Order, std::vector<Handle>> bufferGroupsWithSubOrders =
        std::unordered_map<star::Command_Buffer_Order, std::vector<Handle>>();

    // 0 - no
    // 1 - dynamic submit
    // 2 - standard always submit
    std::vector<unsigned char> bufferSubmissionStatus = std::vector<unsigned char>();

    // Indicates if all semaphores are updated with the proper order of execution
    bool subOrderSemaphoresUpToDate = false;

    void updateSemaphores();
};
} // namespace star