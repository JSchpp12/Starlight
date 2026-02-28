#include "StarRenderPass.hpp"

star::StarRenderPass::~StarRenderPass() {
	device.getVulkanDevice().destroyRenderPass(renderPass); 
}