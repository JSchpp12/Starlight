#pragma once

#include "core/device/managers/ManagerEventBusTies.hpp"

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

constexpr std::string_view GetFenceEventName = "star::core::device::manager::fence";

class Fence : public ManagerEventBusTies<FenceRecord, FenceRequest, 50>
{
  public:
    Fence() : ManagerEventBusTies<FenceRecord, FenceRequest, 50>(common::special_types::FenceTypeName, GetFenceEventName)
    {
    }
    virtual ~Fence() = default;

  protected:
    virtual FenceRecord createRecord(FenceRequest &&request) const
    {
        return FenceRecord{
            .fence = this->m_device->getVulkanDevice().createFence(vk::FenceCreateInfo().setFlags(
                request.createSignalState ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlagBits()))};
    }
};
} // namespace star::core::device::manager