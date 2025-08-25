#pragma once

#include "devices/StarDevice.hpp"

#include <vulkan/vulkan.hpp>

namespace star
{
class StarRenderPass
{
  public:
    StarRenderPass(core::devices::StarDevice &device) : device(device) {};
    virtual ~StarRenderPass();

  protected:
    core::devices::StarDevice &device;
    vk::RenderPass renderPass;
};
} // namespace star