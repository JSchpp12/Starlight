#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>

namespace star {
	struct RenderingTargetInfo {
		RenderingTargetInfo() = default;
		RenderingTargetInfo(const std::vector<vk::Format>& colorAttachmentFormats)
			: colorAttachmentFormats(colorAttachmentFormats) {}
		RenderingTargetInfo(const std::vector<vk::Format>& colorAttachmentFormats, const vk::Format depthAttachmentFormat)
			: colorAttachmentFormats(colorAttachmentFormats), depthAttachmentFormat(depthAttachmentFormat) {}
		RenderingTargetInfo(const std::vector<vk::Format>& colorAttachmentFormats, const vk::Format depthAttachmentFormat, const vk::Format stencilAttachmentFormat)
			: colorAttachmentFormats(colorAttachmentFormats), depthAttachmentFormat(depthAttachmentFormat), stencilAttachmentFormat(stencilAttachmentFormat) {}

		std::vector<vk::Format> colorAttachmentFormats = std::vector<vk::Format>(); 
		std::optional<vk::Format> depthAttachmentFormat = std::optional<vk::Format>();
		std::optional<vk::Format> stencilAttachmentFormat = std::optional<vk::Format>();
	};
}