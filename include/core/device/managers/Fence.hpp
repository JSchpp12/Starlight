#pragma once

#include "core/device/managers/ManagerEventBusTies.hpp"

#include "core/device/system/event/ManagerRequest.hpp"
#include "device/managers/Manager.hpp"

namespace star::core::device::manager
{
struct FenceRequest
{
    bool createSignalState = false;
};
struct FenceRecord
{
    vk::Fence fence;
    bool isReady() const
    {
        return true;
    }
    void cleanupRender(device::StarDevice &device)
    {
        device.getVulkanDevice().destroyFence(fence);
    }
};
class Fence : public ManagerEventBusTies<FenceRecord, FenceRequest, Handle_Type::fence, 50>
{
  public:
    virtual ~Fence() = default;

  protected:
    virtual FenceRecord createRecord(device::StarDevice &device, FenceRequest &&request) const
    {
        return FenceRecord{
            .fence = device.getVulkanDevice().createFence(vk::FenceCreateInfo().setFlags(
                request.createSignalState ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlagBits()))};
    }
};
} // namespace star::core::device::manager