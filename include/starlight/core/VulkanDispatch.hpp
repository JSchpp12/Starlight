#pragma once

#include <vulkan/vulkan.hpp>

namespace star::core
{
void InitializeVulkanDispatchLoader();
void InitializeVulkanInstanceDispatchLoader(vk::Instance instance);
} // namespace star::core
