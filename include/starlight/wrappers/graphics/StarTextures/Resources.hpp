#pragma once

#include "StarTextures/TextureView.hpp"
#include "core/device/StarDevice.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>

namespace star::StarTextures
{
class Resources
{
  public:
    vk::Image image = vk::Image();
    TextureView views = TextureView();
    std::optional<vk::Sampler> sampler = std::nullopt;
    vk::ImageLayout trackedImageLayout = vk::ImageLayout::eUndefined;

    Resources(const vk::Image &image);
    Resources(const vk::Image &image, const TextureView &views);
    Resources(const vk::Image &image, const TextureView &views, const vk::Sampler &sampler);

    virtual void cleanupRender(vk::Device &device);

    virtual ~Resources() = default;
};

} // namespace star::StarTextures