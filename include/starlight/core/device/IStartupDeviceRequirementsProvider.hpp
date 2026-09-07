#pragma once

#include <vulkan/vulkan.hpp>

#include <string_view>
#include <vector>

namespace star::core::device
{

struct PhysicalDeviceFeatureRequest
{
    vk::Bool32 vk::PhysicalDeviceFeatures::*feature;
    std::string_view name;
};

struct DeviceRequirements
{
    /// Required device extension names. Each pointer must remain valid for the duration of startup.
    std::vector<const char *> requiredDeviceExtensions{};
    std::vector<PhysicalDeviceFeatureRequest> physicalDeviceFeatureRequests{};
};

class IStartupDeviceRequirementsProvider
{
  public:
    virtual ~IStartupDeviceRequirementsProvider() = default;

    virtual DeviceRequirements getRequirements() const = 0;
};

} // namespace star::core::device
