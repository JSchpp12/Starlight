#include "BumpMaterial.hpp"

#include <vulkan/vulkan.hpp>

void star::BumpMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
{
	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
	constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void star::BumpMaterial::cleanup(core::device::DeviceContext& device)
{

}

void star::BumpMaterial::buildDescriptorSet(core::device::DeviceContext& context, StarShaderInfo::Builder& builder, const int& imageInFlightIndex)

{
	this->TextureMaterial::buildDescriptorSet(context, builder, imageInFlightIndex);
	builder.add(this->bumpMap, vk::ImageLayout::eShaderReadOnlyOptimal, true);
}

std::vector<std::pair<vk::DescriptorType, const int>> star::BumpMaterial::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, 2)
	};
}
