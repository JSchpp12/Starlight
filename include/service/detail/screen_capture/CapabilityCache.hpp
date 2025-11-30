#pragma once

#include <absl/container/flat_hash_map.h>
#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture
{
struct FormatSupport
{
    bool blitSrcOptimal = false;
    bool blitDstOptimal = false;
    bool blitDstLinear = false; // if you choose linear tiling (often you can keep optimal)
    bool linearFilterOK = false;
};
class CapabilityCache
{
  public:
    CapabilityCache() = default;
    explicit CapabilityCache(vk::PhysicalDevice pd) : m_physicalVulkanDevice(std::move(pd))
    {
    }

    void init(vk::PhysicalDevice pd)
    {
        m_physicalVulkanDevice = std::move(pd);
    }

    const FormatSupport &get(vk::Format format)
    {
        assert(m_physicalVulkanDevice != VK_NULL_HANDLE);

        {
            auto it = m_cache.find(format);
            if (it != m_cache.end())
            {
                return it->second;
            }
        }

        vk::FormatProperties props = m_physicalVulkanDevice.getFormatProperties(format);
        FormatSupport s{
            .blitSrcOptimal = props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc ? true : false,
            .blitDstOptimal = props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst ? true : false,
            .blitDstLinear = props.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst ? true : false,
            .linearFilterOK =  props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear ? true : false
        };
        return m_cache.insert(std::make_pair(format, s)).first->second;
    }

  private:
    vk::PhysicalDevice m_physicalVulkanDevice = VK_NULL_HANDLE;
    absl::flat_hash_map<vk::Format, FormatSupport> m_cache;
};
} // namespace star::service::detail::screen_capture