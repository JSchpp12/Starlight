//wrapper class for textures 
#pragma once

#include "StarTexture.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <string>

namespace star {
	///"Smarter" version of StarTexture which automatically creates and loads data...
class StarImage{
public:
	StarImage(const StarTexture::TextureCreateSettings& createSettings, vk::Device& device,
		const vk::Image& image) 
		: createSettings(createSettings), texture(std::make_unique<StarTexture>(createSettings, device, image))
	{
	}

	StarImage(StarTexture::TextureCreateSettings createSettings) : createSettings(createSettings)
	{
	}

	virtual void prepRender(StarDevice& device);

	virtual ~StarImage() = default;

	void cleanupRender(StarDevice& device); 

	StarTexture& getTexture() {return *this->texture;}
	vk::Sampler getSampler() const { return this->texture->getSampler(); }
	vk::Image getImage() const { return this->texture->getImage(); }
	vk::ImageLayout getCurrentLayout() const { return this->texture->getCurrentLayout(); }
	void overrideImageLayout(vk::ImageLayout newLayout) { this->texture->overrideImageLayout(newLayout); }
	const StarTexture::TextureCreateSettings& getSettings() {return this->createSettings;}



	//std::vector<vk::Format> getFormats() const {
	//	std::vector<vk::Format> formats; 
	//	for (auto& info : this->imageViews) {
	//		formats.push_back(info.first); 
	//	}
	//}
protected:
	//TODO: this is stored in two places, dont need both
	const StarTexture::TextureCreateSettings createSettings;
	std::unique_ptr<StarTexture> texture = nullptr; 

	virtual std::unique_ptr<StarBuffer> loadImageData(StarDevice& device) {
		return nullptr; 
	};

	virtual void createImage(StarDevice& device);

	void transitionImageLayout(StarDevice& device, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout);
};
}