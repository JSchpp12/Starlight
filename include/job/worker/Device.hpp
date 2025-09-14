#pragma once

#include "job/Worker.hpp"

#include <vulkan/vulkan.hpp>

namespace star::job::worker
{
class Device : public Worker
{
  public:
    Device(vk::Device vulkanDevice) : m_vulkanDevice(std::move(vulkanDevice)), Worker()
    {
    }

  private:
    vk::Device m_vulkanDevice;

    virtual void threadFunction() override; 
};
} // namespace star::job::worker