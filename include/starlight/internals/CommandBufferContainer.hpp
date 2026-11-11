#pragma once

#include "MappedHandleContainer.hpp"
#include "StarCommandBuffer.hpp"
#include "device/StarDevice.hpp"
#include "device/managers/Queue.hpp"
#include "starlight/internals/CPURecordingWaitContainer.hpp"
#include "starlight/setup/RenderResourceSetup.hpp"

#include <absl/container/flat_hash_map.h>
#include <boost/container/small_vector.hpp>
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
    class SemaphoreInfo
    {
      public:
        SemaphoreInfo()
        {
            m_semaphores.reserve(16);
        }

        void insert(const Handle &handle, vk::Semaphore semaphore, vk::PipelineStageFlags waitPoints,
                    std::optional<uint64_t> signaledValue = std::nullopt)
        {
            if (m_semaphores.contains(handle))
            {
                m_semaphores[handle].waitPoints |= waitPoints;
            }
            else
            {
                m_semaphores[handle] = StarCommandBuffer::Data{.semaphore = std::move(semaphore),
                                                               .waitPoints = std::move(waitPoints),
                                                               .signaledValue = std::move(signaledValue)};
            }
        }

        void giveMeOneTimeSemaphoreWaitInfo(boost::container::small_vector<StarCommandBuffer::Data, 8> &waitData)
        {
            for (auto &semaphoreInfo : m_semaphores)
            {
                waitData.push_back(semaphoreInfo.second);
            }

            m_semaphores.clear();
        }

        void erase(const Handle &handle)
        {
            m_semaphores.erase(handle);
        }

        void clear()
        {
            m_semaphores.clear();
        }

      private:
        std::unordered_map<Handle, StarCommandBuffer::Data, star::HandleHash> m_semaphores;
    };

    struct CompleteRequest
    {
        struct PerFrameScratchData
        {
            boost::container::small_vector<StarCommandBuffer::Data, 8> data;
        };

        std::function<void(StarCommandBuffer &, const common::FrameTracker &, const uint64_t &)> recordBufferCallback;
        std::unique_ptr<StarCommandBuffer> commandBuffer;
        Queue_Type type;
        StarQueue *queueToUse;
        bool recordOnce;
        vk::PipelineStageFlags waitStage;
        Command_Buffer_Order order;
        std::optional<std::function<void(const int &)>> beforeBufferSubmissionCallback;
        std::optional<StarCommandBuffer::SubmissionOverride> overrideBufferSubmissionCallback;
        SemaphoreInfo oneTimeWaitSemaphoreInfo;
        PerFrameScratchData scratch;

        /// @brief Host-side waits this pass must pass before it records.
        CPURecordingWaitContainer cpuRecordingWaits;

        /// @brief Run every registered wait, clear them, and return false if any resource failed.
        /// @details On failure the failed resources' one-time Vulkan waits are erased, so no wait on a timeline that
        /// will never signal reaches submission. Prepared definitions are left to their normal lifecycle.
        [[nodiscard]] bool runCPURecordingWaits();
        /// @brief Begin and end an empty recording for the given frame-in-flight slot.
        /// @details Used when a readiness wait fails: the pass still submits so group semaphores stay valid.
        void recordEmpty(uint8_t frameInFlightIndex);

        /// @brief One-shot work this pass records inline before its own commands.
        setup::RenderResourceSetup renderResourceSetup;

        /// @brief Adds a definition that CommandOrderService already prepared.
        void add(setup::RenderResourceSetup::ResourcePrepDefinition definition);
        [[nodiscard]] bool isDone() const;
        [[nodiscard]] bool hasWork() const;
        /// @brief Resolves one-shot work's per-frame readiness before the pass records.
        void prepareSetup(const core::device::DeviceContext &context);
        void recordSetup(vk::CommandBuffer commandBuffer, const core::graphics::SemaphoreInfo &recordingGate);
        void pollSetupReleases(core::device::DeviceContext &context);
        void cleanupSetup(core::device::DeviceContext &context);

        ~CompleteRequest();

        CompleteRequest(
            std::function<void(StarCommandBuffer &, const common::FrameTracker &, const uint64_t &)>
                recordBufferCallback,
            std::unique_ptr<StarCommandBuffer> commandBuffer, const Queue_Type &type, const bool &recordOnce,
            const vk::PipelineStageFlags &waitStage, const Command_Buffer_Order &order,
            std::optional<std::function<void(const int &)>> beforeSubmissionCallback =
                std::optional<std::function<void(const int &)>>(),
            std::optional<StarCommandBuffer::SubmissionOverride> overrideBufferSubmissionCallback = std::nullopt);

        vk::Semaphore submitCommandBuffer(core::device::StarDevice &device, const common::FrameTracker &frameTracker,
                                          absl::flat_hash_map<star::Queue_Type, StarQueue *> &queues,
                                          std::vector<vk::Semaphore> *beforeSemaphores = nullptr);
    };

    CommandBufferContainer(core::device::StarDevice &device, const uint8_t &numImagesInFlight);

    void cleanup(core::device::StarDevice &device);

    vk::Semaphore submitGroupWhenReady(core::device::StarDevice &device, const star::Command_Buffer_Order &order,
                                       const common::FrameTracker &frameTracker, const uint64_t &currentFrameIndex,
                                       absl::flat_hash_map<star::Queue_Type, StarQueue *> &queues,
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