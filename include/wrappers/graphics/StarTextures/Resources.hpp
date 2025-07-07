#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <unordered_map>

namespace star::StarTextures
{
class Resources
{
  protected:
    vk::Device &device;

  public:
    vk::Image image = vk::Image();
    std::unordered_map<vk::Format, vk::ImageView> views = std::unordered_map<vk::Format, vk::ImageView>();
    std::optional<vk::Sampler> sampler = std::nullopt;

    Resources(vk::Device &device, const vk::Image &image);
    Resources(vk::Device &device, const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views);
    Resources(vk::Device &device, const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views, const vk::Sampler &sampler); 

    virtual ~Resources();
};

} // namespace star::StarTexture