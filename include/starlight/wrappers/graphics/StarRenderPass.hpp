#pragma once

#include "device/StarDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace star
{
class StarRenderPass
{
  public:
    StarRenderPass(core::device::StarDevice &device) : device(device) {};
    virtual ~StarRenderPass();

  protected:
    core::device::StarDevice &device;
    vk::RenderPass renderPass;
};
} // namespace star