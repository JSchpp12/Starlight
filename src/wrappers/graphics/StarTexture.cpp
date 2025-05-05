#include "StarTexture.hpp"

#include "ConfigFile.hpp"

float star::StarTexture::SelectAnisotropyLevel(const vk::PhysicalDeviceProperties& deviceProperties){
	std::string anisotropySetting = star::ConfigFile::getSetting(star::Config_Settings::texture_anisotropy);
    float anisotropyLevel = 1.0f;

    if (anisotropySetting == "max")
        anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
    else{
        anisotropyLevel = std::stof(anisotropySetting);

        if (anisotropyLevel > deviceProperties.limits.maxSamplerAnisotropy){
            anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
        }else if (anisotropyLevel < 1.0f){
            anisotropyLevel = 1.0f; 
        }
    }

    return anisotropyLevel;
}

vk::Filter star::StarTexture::SelectTextureFiltering(const vk::PhysicalDeviceProperties& deviceProperties){
	auto textureFilteringSetting = ConfigFile::getSetting(Config_Settings::texture_filtering);

    vk::Filter filterType;
    if (textureFilteringSetting == "nearest"){
        filterType = vk::Filter::eNearest;
    }else if (textureFilteringSetting == "linear"){
        filterType = vk::Filter::eLinear;
    }else{
        throw std::runtime_error("Texture filtering setting must be 'nearest' or 'linear'");
    }

    return filterType; 
}

star::StarTexture::~StarTexture(){
	if (this->textureSampler)
		this->device.destroySampler(this->textureSampler);
		
	for (auto& item : this->imageViews) {
		this->device.destroyImageView(item.second);
	}

    if (this->allocator && this->textureMemory){
        vmaDestroyImage(*this->allocator, this->textureImage, this->textureMemory);
	}
}

star::StarTexture::StarTexture(const TextureCreateSettings& createSettings, vk::Device& device, VmaAllocator& allocator) : createSettings(createSettings), allocator(&allocator), device(device){
	assert(this->allocator != nullptr && "Allocator must always exist");

	bool isMutable = false; 
	if (this->createSettings.additionalViewFormats.size() > 0)
		isMutable = true; 

    this->createAllocation(this->createSettings, *this->allocator, this->textureMemory, this->textureImage, isMutable);
	this->createTextureImageView(this->createSettings.baseFormat, this->createSettings.aspectFlags);
	
	for (const auto& viewFormat : this->createSettings.additionalViewFormats){
		this->createTextureImageView(viewFormat, this->createSettings.aspectFlags);
	}

	if (this->createSettings.createSampler)
		this->textureSampler = this->createImageSampler(device, this->createSettings);
}

star::StarTexture::StarTexture(const TextureCreateSettings& createSettings, vk::Device& device, const vk::Image& textureImage) : createSettings(createSettings), textureImage(textureImage), device(device){
	this->createTextureImageView(this->createSettings.baseFormat, this->createSettings.aspectFlags);
	for (const auto& viewFormat : this->createSettings.additionalViewFormats){
		this->createTextureImageView(viewFormat, createSettings.aspectFlags); 
	}
}

void star::StarTexture::transitionLayout(vk::CommandBuffer& commandBuffer, vk::ImageLayout newLayout, vk::AccessFlags srcFlags, vk::AccessFlags dstFlags, vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags dstStage){
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;
	barrier.oldLayout = this->layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

	barrier.image = this->textureImage;
	barrier.srcAccessMask = srcFlags;
	barrier.dstAccessMask = dstFlags;

	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
	barrier.subresourceRange.levelCount = 1;                            //image is not an array
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	commandBuffer.pipelineBarrier(
		sourceStage,                        //which pipeline stages should occurr before barrier 
		dstStage,                   //pipeline stage in which operations will wait on the barrier 
		{},
		{},
		nullptr,
		barrier
	);

	this->layout = newLayout; 
}

vk::DeviceSize star::StarTexture::getImageMemorySize() const {
	if (this->createSettings.overrideImageMemorySize.has_value()){
		return this->createSettings.overrideImageMemorySize.value(); 
	}else{
		return vk::DeviceSize(this->createSettings.width * this->createSettings.height * this->createSettings.depth * this->createSettings.byteDepth);
	}
}

void star::StarTexture::createAllocation(const TextureCreateSettings& createSettings, VmaAllocator& allocator, VmaAllocation& textureMemory, vk::Image& image, const bool& isMutable){
    /* Create vulkan image */
	vk::ImageCreateInfo imageInfo{};
	imageInfo.sType = vk::StructureType::eImageCreateInfo;

	if (createSettings.depth > 1) {
		imageInfo.imageType = vk::ImageType::e3D;
		imageInfo.flags = vk::ImageCreateFlagBits::e2DArrayCompatible;
	}
	else
		imageInfo.imageType = vk::ImageType::e2D;

	if (isMutable)
		imageInfo.flags = vk::ImageCreateFlagBits::eMutableFormat; 

	imageInfo.extent.width = createSettings.width;
	imageInfo.extent.height = createSettings.height;
	imageInfo.extent.depth = createSettings.depth;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = createSettings.baseFormat;
	imageInfo.tiling = vk::ImageTiling::eOptimal;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = createSettings.usage;
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

	std::string fullAllocationName = std::string(createSettings.allocationName);
	fullAllocationName += "_TEXTURE";
	vmaSetAllocationName(allocator, textureMemory, fullAllocationName.c_str()); 
}

vk::ImageView star::StarTexture::getImageView(const vk::Format* requestedFormat) const{
	if (requestedFormat != nullptr){
		//make sure the image view actually exists for the requested format
		assert(this->imageViews.find(*requestedFormat) != this->imageViews.end() && "The image must be created with the proper image view before being requested");
		return this->imageViews.at(*requestedFormat);
	}

	//just grab the first one
	for (auto& view : this->imageViews) {
		return view.second; 
	}
}

void star::StarTexture::createTextureImageView(const vk::Format& viewFormat, const vk::ImageAspectFlags& aspectFlags) {
	if (this->imageViews.find(viewFormat) == this->imageViews.end()){
		vk::ImageView imageView = createImageView(this->device, this->textureImage, viewFormat, aspectFlags);
		this->imageViews.insert(std::pair<vk::Format, vk::ImageView>(viewFormat, imageView)); 
	}
}

vk::ImageView star::StarTexture::createImageView(vk::Device& device, vk::Image image, vk::Format format, const vk::ImageAspectFlags& aspectFlags) {
	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	viewInfo.image = image;
	if (this->createSettings.depth > 1)
		viewInfo.viewType = vk::ImageViewType::e3D;
	else {
		viewInfo.viewType = vk::ImageViewType::e2D;
	}
	viewInfo.format = format;

	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView = device.createImageView(viewInfo);

	if (!imageView) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

vk::Sampler star::StarTexture::createImageSampler(vk::Device& device, const star::StarTexture::TextureCreateSettings& createSettings){
	vk::SamplerCreateInfo samplerInfo{}; 
	samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;

	if (createSettings.anisotropyLevel != 0.0f){
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = createSettings.anisotropyLevel;
	}

	samplerInfo.magFilter = createSettings.textureFilteringMode;
	samplerInfo.minFilter = createSettings.textureFilteringMode;

	//repeat mode - repeat the texture when going beyond the image dimensions
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

	
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	//specifies coordinate system to use in addressing texels. 
	//VK_TRUE - use coordinates [0, texWidth) and [0, texHeight]
	//VK_FALSE - use [0, 1)
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	//if comparing, the texels will first compare to a value, the result of the comparison is used in filtering operations (percentage-closer filtering on shadow maps)
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	
	//following apply to mipmapping -- not using here
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	vk::Sampler textureSampler = device.createSampler(samplerInfo);
	if (!textureSampler)
		throw std::runtime_error("failed to create texture sampler");

	return textureSampler; 
}