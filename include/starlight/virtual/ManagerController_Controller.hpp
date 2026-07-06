#pragma once

#include "StarCommandBuffer.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace star::ManagerController
{

template <typename TTransferType, typename TDataType> class Controller
{
  public:
    Controller() = default;
    virtual ~Controller() = default;

    bool willBeUpdatedThisFrame(const uint64_t &currentFrameCount, const uint8_t &currentFrameInFlightIndex) const
    {
        return hasAlreadyBeenUpdatedThisFrame(currentFrameCount) ||
               doesFrameInFlightDataNeedUpdated(currentFrameInFlightIndex);
    }

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
    {
        if (m_resourceHandles.size() == 0)
        {
            m_resourceHandles.resize(numFramesInFlight);

            for (uint8_t i = 0; i < numFramesInFlight; i++)
            {
                m_resourceHandles[i] = context.getManagerRenderResource().addRequest(context.getDeviceID());
            }
        }
    }

    Handle getHandle(const uint8_t &index) const
    {
        assert(index <= m_resourceHandles.size() && "Ensure index is within range of requested resource size");

        return m_resourceHandles[index];
    }

    /// Call any frame updates. Returns true if the controller submitted an update
    std::pair<bool, const star::StarSemaphore *> submitUpdateIfNeeded(
        core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
        std::optional<star::core::graphics::SemaphoreInfo> transferGPUWorkWaitOnSyncInfo = std::nullopt)
    {
        const size_t fi = static_cast<size_t>(context.frameTracker().getCurrent().getFrameInFlightIndex());

        assert(fi < m_resourceHandles.size() && m_resourceHandles[fi].isInitialized() &&
               "Resources must be properly prepared before use");
        if (!doesFrameInFlightDataNeedUpdated(fi))
            return std::make_pair(false, nullptr);

        if (hasAlreadyBeenUpdatedThisFrame(context.frameTracker().getCurrent().getGlobalFrameCounter()))
            return std::make_pair(true, &getSemaphore(context, fi));

        m_lastFrameUpdate = context.frameTracker().getCurrent().getGlobalFrameCounter();
        context.getManagerRenderResource().updateRequest(
            context.getDeviceID(), createTransferRequest(context, frameInFlightIndex),
            m_resourceHandles[frameInFlightIndex], std::move(transferGPUWorkWaitOnSyncInfo), true);
        return std::make_pair(true, &getSemaphore(context, fi));
    }

    const star::StarSemaphore &getSemaphore(star::core::device::DeviceContext &context, size_t frameInFlightIndex) const
    {
        const auto *record = context.getManagerRenderResource().get<TDataType>(context.getDeviceID(),
                                                                               m_resourceHandles[frameInFlightIndex]);

        assert(record != nullptr && "Failed to get record from managerRenderResource during request for semaphore");

        return record->gpuWorkDoneSignaledInfo;
    }

  protected:
    uint64_t m_lastFrameUpdate = 10000000; // counter for last index in which data was updated. Initialized to arb large
                                           // value to avoid start at 0.
    std::vector<Handle> m_resourceHandles = std::vector<Handle>();

    virtual std::unique_ptr<TTransferType> createTransferRequest(core::device::DeviceContext &context,
                                                                 const uint8_t &frameInFlightIndex) = 0;

    bool hasAlreadyBeenUpdatedThisFrame(const uint64_t &currentFrameCount) const
    {
        return m_lastFrameUpdate == currentFrameCount;
    }

    virtual bool doesFrameInFlightDataNeedUpdated(const uint8_t &frameInFlightIndex) const = 0;
};
} // namespace star::ManagerController