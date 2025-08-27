#pragma once

#include "DeviceContext.hpp"

#include <vulkan/vulkan.hpp>

#include <functional>
#include <vector>

namespace star
{
class DescriptorModifier
{
  public:
    DescriptorModifier()
    {
        this->submitToManager();
    }

    virtual ~DescriptorModifier()
    {
    }

  protected:
    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        const int &numFramesInFlight) = 0;

    virtual void createDescriptors(core::device::DeviceContext &device, const int &numFramesInFlight) = 0;

  private:
    void submitToManager();
};
} // namespace star