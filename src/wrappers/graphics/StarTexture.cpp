#include "StarTexture.hpp"

namespace star {
StarTexture::~StarTexture() {

}

void StarTexture::prepRender(StarDevice& device) {
	assert(!this->isRenderReady && "Each texture should only be prepared once"); 

	createTextureImage(device);
	createTextureImageView(device);
	createImageSampler(device);

	this->createSettings.release(); 
	this->isRenderReady = true; 
}

void StarTexture::cleanupRender(StarDevice& device)
{
	assert(this->isRenderReady && "Only textures which are ready for rendering operations need to be cleaned up"); 

	device.getDevice().destroySampler(this->textureSampler);
	for (auto& item : this->imageViews) {
		device.getDevice().destroyImageView(item.second);
	}

	device.getDevice().destroyImage(this->textureImage);
	device.getDevice().freeMemory(this->imageMemory);

	this->isRenderReady = false; 
}

vk::ImageView StarTexture::getImageView(vk::Format* requestedFormat){
	if (requestedFormat != nullptr) {
		//make sure the image view actually exists for the requested format
		assert(this->imageViews.find(*requestedFormat) != this->imageViews.end() && "The image must be created with the proper image view before being requested");
		return this->imageViews.at(*requestedFormat);
	}else {
		//just grab the first one
		for (auto& view : this->imageViews) {
			return view.second; 
		}
	}
}

void StarTexture::createTextureImage(StarDevice& device) {
	int height = this->getHeight(); 
	int width = this->getWidth(); 
	int channels = this->getChannels(); 
	bool isMutable = false; 

	if (this->createSettings->viewFormats.size() > 1) {
		isMutable = true; 
	}

	//image has data in cpu memory, it must be copied over
	if (this->data().get()[0] != '\0') {
		vk::DeviceSize imageSize = width * height * 4;

		//image will be transfered from cpu memory, make sure proper flags are set
		this->createSettings->imageUsage = this->createSettings->imageUsage | vk::ImageUsageFlagBits::eTransferDst;

		createImage(device, width, height, this->createSettings->imageFormat, vk::ImageTiling::eOptimal, this->createSettings->imageUsage, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, imageMemory, isMutable);

		auto data = this->data();
		StarBuffer stagingBuffer(
			device,
			imageSize,
			1,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		stagingBuffer.map();
		std::unique_ptr<unsigned char> textureData(this->data());
		stagingBuffer.writeToBuffer(textureData.get(), imageSize);

		//copy staging buffer to texture image 
		transitionImageLayout(device, textureImage, this->createSettings->imageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		device.copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		//prepare final image for texture mapping in shaders 
		transitionImageLayout(device, textureImage, this->createSettings->imageFormat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		this->layout = vk::ImageLayout::eTransferDstOptimal; 
	}
	else {
		createImage(device, width, height, this->createSettings->imageFormat, vk::ImageTiling::eOptimal, this->createSettings->imageUsage, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, imageMemory, isMutable);
	}
}

void StarTexture::createImage(StarDevice& device, uint32_t width, uint32_t height, vk::Format format,
	vk::ImageTiling tiling, vk::ImageUsageFlags usage,
	vk::MemoryPropertyFlagBits properties, vk::Image& image, vk::DeviceMemory& imageMemory, bool isMutable) {

	/* Create vulkan image */
	vk::ImageCreateInfo imageInfo{};
	imageInfo.sType = vk::StructureType::eImageCreateInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;

	if (isMutable)
		imageInfo.flags = vk::ImageCreateFlagBits::eMutableFormat; 

	device.verifyImageCreate(imageInfo);

	image = device.getDevice().createImage(imageInfo);
	if (!image) {
		throw std::runtime_error("failed to create image");
	}

	/* Allocate the memory for the imag*/
	vk::MemoryRequirements memRequirements = device.getDevice().getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);

	imageMemory = device.getDevice().allocateMemory(allocInfo);
	if (!imageMemory) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	device.getDevice().bindImageMemory(image, imageMemory, 0);
}

void StarTexture::transitionLayout(vk::CommandBuffer& commandBuffer, 
	vk::ImageLayout newLayout, vk::AccessFlags srcFlags, vk::AccessFlags dstFlags, 
	vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags dstStage)
{
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;
	barrier.oldLayout = this->layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

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

void StarTexture::transitionImageLayout(StarDevice& device, vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
	vk::ImageLayout newLayout) {
	vk::CommandBuffer commandBuffer = device.beginSingleTimeCommands();

	//create a barrier to prevent pipeline from moving forward until image transition is complete
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;     //specific flag for image operations
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	//if barrier is used for transferring ownership between queue families, this would be important -- set to ignore since we are not doing this
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
	barrier.subresourceRange.levelCount = 1;                            //image is not an array
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	//the operations that need to be completed before and after the barrier, need to be defined
	barrier.srcAccessMask = {}; //TODO
	barrier.dstAccessMask = {}; //TODO

	vk::PipelineStageFlags sourceStage, destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		//undefined transition state, dont need to wait for this to complete
		barrier.srcAccessMask = {};
		//barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		//transfer destination shader reading, will need to wait for completion. Especially in the frag shader where reads will happen
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		//preparing to update texture during runtime, need to wait for top of pipe
		//barrier.srcAccessMask = vk::AccessFlagBits::;
		barrier.srcAccessMask = {}; 
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead; 

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else{
		throw std::invalid_argument("unsupported layout transition!");
	}

	//transfer writes must occurr during the pipeline transfer stage
	commandBuffer.pipelineBarrier(
		sourceStage,                        //which pipeline stages should occurr before barrier 
		destinationStage,                   //pipeline stage in which operations will wait on the barrier 
		{},
		{},
		nullptr,
		barrier
	);

	device.endSingleTimeCommands(commandBuffer);
}

void StarTexture::createImageSampler(StarDevice& device) {
	//get device properties for amount of anisotropy permitted
	vk::PhysicalDeviceProperties deviceProperties = device.getPhysicalDevice().getProperties();

	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;
	samplerInfo.magFilter = vk::Filter::eLinear;                       //how to sample textures that are magnified 
	samplerInfo.minFilter = vk::Filter::eLinear;                       //how to sample textures that are minified
	
	//repeat mode - repeat the texture when going beyond the image dimensions
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;

	//should anisotropic filtering be used? Really only matters if performance is a concern
	samplerInfo.anisotropyEnable = VK_TRUE;
	//specifies the limit on the number of texel samples that can be used (lower = better performance)
	samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;;
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
	samplerInfo.anisotropyEnable = VK_FALSE;

	this->textureSampler = device.getDevice().createSampler(samplerInfo);
	if (!this->textureSampler) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void StarTexture::createTextureImageView(StarDevice& device) {
	for (auto& format : this->createSettings->viewFormats) {
		auto imageView = createImageView(device, textureImage, this->createSettings->imageFormat, vk::ImageAspectFlagBits::eColor);
		this->imageViews.insert(std::pair<vk::Format, vk::ImageView>(format, imageView)); 
	}
}

vk::ImageView StarTexture::createImageView(StarDevice& device, vk::Image image, 
	vk::Format format, vk::ImageAspectFlagBits aspectFlags) {
	vk::ImageViewCreateInfo viewInfo{};
	viewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = format;

	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView = device.getDevice().createImageView(viewInfo);

	if (!imageView) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}
}