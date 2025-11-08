#pragma once

#include "core/device/StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <unordered_map>

namespace star::StarTextures
{
class Resources
{
  public:
    vk::Image image = vk::Image();
    std::unordered_map<vk::Format, vk::ImageView> views = std::unordered_map<vk::Format, vk::ImageView>();
    std::optional<vk::Sampler> sampler = std::nullopt;

    Resources(const vk::Image &image);
    Resources(const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views);
    Resources(const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views,
              const vk::Sampler &sampler);

    virtual void cleanupRender(vk::Device &device);

    virtual ~Resources() = default;
};

} // namespace star::StarTextures