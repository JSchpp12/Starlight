#include "StarRenderMaterial.hpp"

namespace star {
#pragma region Builder
StarRenderMaterial::Builder& StarRenderMaterial::Builder::setMaterial(Handle materialHandle) {
	assert(material == nullptr && "A material has already been applied to this object");

	material = &materialManager.resource(materialHandle);

	return *this;
}
std::unique_ptr<StarRenderMaterial> StarRenderMaterial::Builder::build() {
	assert(material != nullptr && "A material object is required to create a StarRenderMaterial object");
	return std::make_unique<StarRenderMaterial>(starDevice, *material,
		textureManager.resource(material->texture),
		material->bumpMap.type == Handle_Type::texture ? textureManager.resource(material->bumpMap) : mapManager.resource(material->bumpMap));
}
#pragma endregion
StarRenderMaterial::StarRenderMaterial(StarDevice& starDevice, Material& material, Texture& texture, Texture& bumpMap)
	: starDevice(starDevice), material(material),
	texture(std::make_unique<StarTexture>(starDevice, texture)),
	bumpMap(std::make_unique<StarTexture>(starDevice, bumpMap)) {
}

void StarRenderMaterial::initDescriptorLayouts(StarDescriptorSetLayout::Builder& constBuilder) {
	constBuilder.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
	constBuilder.addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
}

void StarRenderMaterial::buildConstDescriptor(StarDescriptorWriter writer) {
	auto texInfo = vk::DescriptorImageInfo{
		texture->getSampler(),
		texture->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(0, texInfo);

	auto bumpInfo = vk::DescriptorImageInfo{
		bumpMap->getSampler(),
		bumpMap->getImageView(),
		vk::ImageLayout::eShaderReadOnlyOptimal };
	writer.writeImage(1, bumpInfo);

	writer.build(descriptor);
}

void StarRenderMaterial::bind(vk::CommandBuffer& commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex) {
	if (this->descriptor) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2, 1, &this->descriptor, 0, nullptr);
	}
}
}