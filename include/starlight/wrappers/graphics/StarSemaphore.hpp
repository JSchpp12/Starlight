#pragma once

#include <vulkan/vulkan.hpp>

namespace star
{
class StarSemaphore
{
    vk::Semaphore m_semaphore{VK_NULL_HANDLE};
    uint64_t previousSignaledValue{0};

  public:
    StarSemaphore() = default;
    void prepRender(vk::Device device); 
    void cleanupRender(vk::Device device); 
};
}