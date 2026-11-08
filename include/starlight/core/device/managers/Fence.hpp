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

constexpr std::string_view GetFenceEventName = "stFenE";

/// @brief Compile-time name of the fence resource type, used for handle registration and failure logging.
struct FenceResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return common::special_types::FenceTypeName;
    }
};

class Fence : public ManagerEventBusTies<FenceRecord, FenceRequest, 50, FenceResourceTypeName>
{
  public:
    Fence() : ManagerEventBusTies<FenceRecord, FenceRequest, 50, FenceResourceTypeName>(GetFenceEventName)
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