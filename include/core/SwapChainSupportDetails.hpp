#pragma once

#include <vulkan/vulkan.hpp>

#include<vector>

namespace star::core
{
struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};
} // namespace star::core