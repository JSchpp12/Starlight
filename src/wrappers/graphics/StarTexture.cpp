#include "StarTexture.hpp"

star::StarTexture::~StarTexture(){
    if (this->allocator != nullptr && this->textureMemory)
        vmaDestroyImage(*this->allocator, this->textureImage, this->textureMemory);
}

star::StarTexture::StarTexture(const TextureCreateSettings& createSettings) : createSettings(createSettings){
	
}

star::StarTexture::StarTexture(const TextureCreateSettings& createSettings, VmaAllocator& allocator) : createSettings(createSettings), allocator(&allocator){
    assert(this->allocator != nullptr && "Allocator must always exist");

    this->createAllocation(this->createSettings, *this->allocator, this->textureMemory, this->textureImage);
}

void star::StarTexture::prepRender(StarDevice& device) {
	this->allocator = &device.getAllocator().get();
	this->createAllocation(this->createSettings, *this->allocator, this->textureMemory, this->textureImage);
}


star::StarTexture::StarTexture(const TextureCreateSettings& createSettings, const vk::Image& image) : createSettings(createSettings), textureImage(image){

}

void star::StarTexture::createAllocation(const TextureCreateSettings& createSettings, VmaAllocator& allocator, VmaAllocation& textureMemory, vk::Image& image){
    /* Create vulkan image */
	vk::ImageCreateInfo imageInfo{};
	imageInfo.sType = vk::StructureType::eImageCreateInfo;

	if (createSettings.depth > 1) {
		imageInfo.imageType = vk::ImageType::e3D;
		imageInfo.flags = vk::ImageCreateFlagBits::e2DArrayCompatible;
	}
	else
		imageInfo.imageType = vk::ImageType::e2D;

	imageInfo.extent.width = createSettings.width;
	imageInfo.extent.height = createSettings.height;
	imageInfo.extent.depth = createSettings.depth;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = createSettings.imageFormat;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = createSettings.imageUsage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;


    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = createSettings.allocationCreateFlags;
    allocInfo.usage = createSettings.memoryUsage;
    if (createSettings.requiredMemoryProperties.has_value()) {
        allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(createSettings.requiredMemoryProperties.value());
    }

    auto result = vmaCreateImage(allocator, (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&image, &textureMemory, nullptr);

    if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to create image: " + result);
    }
}
