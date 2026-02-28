//#include "HeightDisplacementMaterial.hpp"
//
//void star::HeightDisplacementMaterial::prep(StarDevice& device)
//{
//	texture->prepRender(device);
//}
//
//void star::HeightDisplacementMaterial::applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder)
//{
//	constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
//}
//
//void star::HeightDisplacementMaterial::cleanup(StarDevice& device)
//{
//	this->texture.reset();
//}
//
//void star::HeightDisplacementMaterial::buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& frameInFlightIndex)
//{
//	builder.add(*texture, vk::ImageLayout::eShaderReadOnlyOptimal); 
//}
