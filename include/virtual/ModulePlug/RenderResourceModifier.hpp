#pragma once

#include "DeviceContext.hpp"

#include <vulkan/vulkan.hpp>

namespace star
{
class RenderResourceModifier
{
  public:
    RenderResourceModifier()
    {
        this->registerRenderResourceCallbacks();
    }

    virtual void initResources(core::devices::DeviceContext &device, const int &numFramesInFlight,
                               const vk::Extent2D &screenSize) = 0;

    virtual void destroyResources(core::devices::DeviceContext &device) = 0;

  private:
    void registerRenderResourceCallbacks();
};
} // namespace star