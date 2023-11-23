#include "StarRenderPass.hpp"

star::StarRenderPass::~StarRenderPass() {
	device.getDevice().destroyRenderPass(renderPass); 
}