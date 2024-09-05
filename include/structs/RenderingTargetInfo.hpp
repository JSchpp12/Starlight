#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>

namespace star {
	struct RenderingTargetInfo {
		RenderingTargetInfo() = default; 
		~RenderingTargetInfo() = default;

		std::vector<vk::Format> colorAttachmentFormats; 
		std::optional<vk::Format> depthAttachmentFormat;
		std::optional<vk::Format> stencilAttachmentFormat; 
	};
}