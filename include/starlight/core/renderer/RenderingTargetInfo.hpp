#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>


namespace star::core::renderer
{
struct RenderingTargetInfo
{
    std::vector<vk::Format> colorAttachmentFormats = std::vector<vk::Format>();
    std::optional<vk::Format> depthAttachmentFormat = std::optional<vk::Format>();
    std::optional<vk::Format> stencilAttachmentFormat = std::optional<vk::Format>();
};
} // namespace star::core::renderer