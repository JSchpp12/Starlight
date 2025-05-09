#include "StarTexture.hpp"

#include "ConfigFile.hpp"

#include <cassert>
#include <stdexcept>

void star::StarTexture::TransitionImageLayout(StarTexture &image, vk::CommandBuffer &commandBuffer, const vk::Format &format, const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout)
{
	//create a barrier to prevent pipeline from moving forward until image transition is complete
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;     //specific flag for image operations
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	//if barrier is used for transferring ownership between queue families, this would be important -- set to ignore since we are not doing this
	barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

	barrier.image = image.getVulkanImage();
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = image.getMipmapLevels();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	//the operations that need to be completed before and after the barrier, need to be defined
	barrier.srcAccessMask = {}; //TODO
	barrier.dstAccessMask = {}; //TODO

	vk::PipelineStageFlags sourceStage, destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		//undefined transition state, dont need to wait for this to complete
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal || newLayout == vk::ImageLayout::eGeneral)) {
		//transfer destination shader reading, will need to wait for completion. Especially in the frag shader where reads will happen
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eNone;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	// else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal) {
	// 	//preparing to update texture during runtime, need to wait for top of pipe
	// 	//barrier.srcAccessMask = vk::AccessFlagBits::;
	// 	barrier.srcAccessMask = {}; 
	// 	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead; 

	// 	sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
	// 	destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	// }
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
}

float star::StarTexture::SelectAnisotropyLevel(const vk::PhysicalDeviceProperties &deviceProperties)
{
    std::string anisotropySetting = star::ConfigFile::getSetting(star::Config_Settings::texture_anisotropy);
    float anisotropyLevel = 1.0f;

    if (anisotropySetting == "max")
        anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
    else
    {
        anisotropyLevel = std::stof(anisotropySetting);

        if (anisotropyLevel > deviceProperties.limits.maxSamplerAnisotropy)
        {
            anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
        }
        else if (anisotropyLevel < 1.0f)
        {
            anisotropyLevel = 1.0f;
        }
    }

    return anisotropyLevel;
}

vk::Filter star::StarTexture::SelectTextureFiltering(const vk::PhysicalDeviceProperties &deviceProperties)
{
    auto textureFilteringSetting = ConfigFile::getSetting(Config_Settings::texture_filtering);

    vk::Filter filterType;
    if (textureFilteringSetting == "nearest")
    {
        filterType = vk::Filter::eNearest;
    }
    else if (textureFilteringSetting == "linear")
    {
        filterType = vk::Filter::eLinear;
    }
    else
    {
        throw std::runtime_error("Texture filtering setting must be 'nearest' or 'linear'");
    }

    return filterType;
}

star::StarTexture::~StarTexture()
{
    if (this->sampler)
        this->device.destroySampler(this->sampler);

    for (auto &item : this->views)
    {
        this->device.destroyImageView(item.second);
    }

    if (this->allocator.has_value() && this->memory.has_value())
    {
        vmaDestroyImage(*this->allocator.value(), this->vulkanImage, this->memory.value());
    }
}

vk::ImageView star::StarTexture::getImageView(const vk::Format *requestedFormat) const
{
    if (requestedFormat != nullptr)
    {
        // make sure the image view actually exists for the requested format
        assert(this->views.find(*requestedFormat) != this->views.end() &&
               "The image must be created with the proper image view before being requested");
        return this->views.at(*requestedFormat);
    }

    // just grab the first one
    for (auto &view : this->views)
    {
        return view.second;
    }
}

star::StarTexture::StarTexture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                               const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::Format& baseFormat,
                               const vk::SamplerCreateInfo &samplerInfo)
    : device(device), allocator(&allocator), memory(VmaAllocation()), baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
    CreateAllocation(device, allocator, allocationCreateInfo, createInfo, this->memory.value(),
                     this->vulkanImage, allocationName);

    this->views = CreateImageViews(device, this->vulkanImage, imageViewInfos);
    this->sampler = CreateImageSampler(device, samplerInfo);
}

star::StarTexture::StarTexture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                               const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, const vk::Format& baseFormat)
    : device(device), allocator(&allocator), memory(VmaAllocation()), baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
    CreateAllocation(device, allocator, allocationCreateInfo, createInfo, this->memory.value(),
                     this->vulkanImage, allocationName);

    this->views = CreateImageViews(device, this->vulkanImage, imageViewInfos);
}

star::StarTexture::StarTexture(vk::Device &device, vk::Image &vulkanImage,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
							   const vk::Format& baseFormat,
                               const vk::SamplerCreateInfo &samplerInfo)
    : device(device), vulkanImage(vulkanImage), baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
    this->views = CreateImageViews(device, this->vulkanImage, imageViewInfos);
    this->sampler = CreateImageSampler(device, samplerInfo);
}

star::StarTexture::StarTexture(vk::Device &device, vk::Image &vulkanImage,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos, 
							   const vk::Format& baseFormat)
    : device(device), vulkanImage(vulkanImage), baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
    this->views = CreateImageViews(device, this->vulkanImage, imageViewInfos);
}

vk::Sampler star::StarTexture::CreateImageSampler(vk::Device &device, const vk::SamplerCreateInfo &samplerCreateInfo)
{
    vk::Sampler sampler = device.createSampler(samplerCreateInfo);

    if (!sampler)
    {
        std::cerr << "Failed to create sampler" << std::endl;
        throw std::runtime_error("Failed to create sampler");
    }

    return sampler;
}

void star::StarTexture::CreateAllocation(vk::Device &device, VmaAllocator &allocator,
                                         const VmaAllocationCreateInfo &allocationCreateInfo,
                                         const vk::ImageCreateInfo &imageCreateInfo, VmaAllocation &allocation,
                                         vk::Image &textureImage, const std::string &allocationName)
{
    auto result = vmaCreateImage(allocator, (VkImageCreateInfo *)&imageCreateInfo, &allocationCreateInfo,
                                 (VkImage *)&textureImage, &allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image: " + result);
    }

    std::string fullAllocationName = std::string(allocationName) + "_TEXTURE";
    vmaSetAllocationName(allocator, allocation, fullAllocationName.c_str());
}

std::unordered_map<vk::Format, vk::ImageView> star::StarTexture::CreateImageViews(
    vk::Device &device, vk::Image vulkanImage, std::vector<vk::ImageViewCreateInfo> imageCreateInfos)
{
    assert(imageCreateInfos.size() > 0 && "An image view is required for every image");

    std::unordered_map<vk::Format, vk::ImageView> nViews = std::unordered_map<vk::Format, vk::ImageView>();

    for (auto &info : imageCreateInfos)
    {
        vk::Format nFormat = info.format;
		info.image = vulkanImage; 

        vk::ImageView nView = CreateImageView(device, info);

        nViews.insert(std::make_pair(nFormat, nView));
    }

    return nViews;
}

vk::ImageView star::StarTexture::CreateImageView(vk::Device &device, const vk::ImageViewCreateInfo &imageCreateInfo)
{
    vk::ImageView imageView = device.createImageView(imageCreateInfo);

    if (!imageView)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

uint32_t star::StarTexture::ExtractMipmapLevels(const vk::ImageViewCreateInfo &imageViewInfo)
{
	return imageViewInfo.subresourceRange.levelCount; 
}

star::StarTexture::Builder::Builder(vk::Device &device, const vk::Image &vulkanImage)
    : device(device), vulkanImage(vulkanImage)
{
}

star::StarTexture::Builder::Builder(vk::Device &device, VmaAllocator &allocator)
    : device(device), createNewAllocationInfo{std::make_optional<Creators>(allocator)}
{
}

star::StarTexture::Builder &star::StarTexture::Builder::setCreateInfo(const VmaAllocationCreateInfo &nAllocInfo,
                                                                      const vk::ImageCreateInfo &nCreateInfo,
                                                                      const std::string &nAllocName)
{
    assert(this->createNewAllocationInfo.has_value() && "Must be a builder for a new allocation");

    this->createNewAllocationInfo.value().createInfo = nCreateInfo;
    this->createNewAllocationInfo.value().allocationCreateInfo = nAllocInfo;
    this->createNewAllocationInfo.value().allocationName = nAllocName;
    return *this;
}

star::StarTexture::Builder &star::StarTexture::Builder::addViewInfo(const vk::ImageViewCreateInfo &nCreateInfo)
{
    this->viewInfos.push_back(nCreateInfo);
    return *this;
}

star::StarTexture::Builder &star::StarTexture::Builder::setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo)
{
    this->samplerInfo = nCreateInfo;
    return *this;
}

star::StarTexture::Builder &star::StarTexture::Builder::setBaseFormat(const vk::Format &nFormat)
{
	this->format = nFormat; 
	return *this;
}

std::unique_ptr<star::StarTexture> star::StarTexture::Builder::build()
{
	assert(this->format.has_value()); 

    if (this->createNewAllocationInfo.has_value())
    {
        // assert(this->createNewAllocationInfo.value().createInfo &&
        // this->createNewAllocationInfo.value().allocationCreateInfo && "Other build info must be provided for
        // allocation creation");

        if (this->samplerInfo.has_value())
        {
            return std::unique_ptr<StarTexture>(new StarTexture(
                this->device, this->createNewAllocationInfo.value().allocator,
                this->createNewAllocationInfo.value().createInfo,
                this->createNewAllocationInfo.value().allocationName, 
                this->createNewAllocationInfo.value().allocationCreateInfo,
				this->viewInfos, 
				this->format.value(), 
				this->samplerInfo.value()));
        }
        else
        {
            return std::unique_ptr<StarTexture>(
                new StarTexture(this->device, this->createNewAllocationInfo.value().allocator,
                                this->createNewAllocationInfo.value().createInfo,
                                this->createNewAllocationInfo.value().allocationName,
                                this->createNewAllocationInfo.value().allocationCreateInfo,
                                 this->viewInfos, this->format.value()));
        }
    }
    else if (this->vulkanImage.has_value())
    {
        if (this->samplerInfo.has_value())
        {
            return std::unique_ptr<StarTexture>(
                new StarTexture(this->device, this->vulkanImage.value(), this->viewInfos, this->format.value()));
        }
        else
        {
            return std::unique_ptr<StarTexture>(
                new StarTexture(this->device, this->vulkanImage.value(), this->viewInfos, this->format.value(), this->samplerInfo.value()));
        }
    }
    else
    {
        std::cerr << "Invalid builder config" << std::endl;
        throw std::runtime_error("Invalid builder config");
    }
    return nullptr;
}