#include "StarTextures/Texture.hpp"

#include "ConfigFile.hpp"
#include "StarTextures/AllocatedResources.hpp"

#include <cassert>
#include <stdexcept>

void star::StarTextures::Texture::TransitionImageLayout(Texture &image, vk::CommandBuffer &commandBuffer,
                                                        const vk::Format &format, const vk::ImageLayout &oldLayout,
                                                        const vk::ImageLayout &newLayout)
{
    // create a barrier to prevent pipeline from moving forward until image transition is complete
    vk::ImageMemoryBarrier barrier{};
    barrier.sType = vk::StructureType::eImageMemoryBarrier; // specific flag for image operations
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    // if barrier is used for transferring ownership between queue families, this would be important -- set to ignore
    // since we are not doing this
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

    barrier.image = image.getVulkanImage();
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image.getMipmapLevels();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // the operations that need to be completed before and after the barrier, need to be defined
    barrier.srcAccessMask = {}; // TODO
    barrier.dstAccessMask = {}; // TODO

    vk::PipelineStageFlags sourceStage, destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        // undefined transition state, dont need to wait for this to complete
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal || newLayout == vk::ImageLayout::eGeneral))
    {
        // transfer destination shader reading, will need to wait for completion. Especially in the frag shader where
        // reads will happen
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
    }
    // else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout ==
    // vk::ImageLayout::eTransferDstOptimal) {
    // 	//preparing to update texture during runtime, need to wait for top of pipe
    // 	//barrier.srcAccessMask = vk::AccessFlagBits::;
    // 	barrier.srcAccessMask = {};
    // 	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    // 	sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    // 	destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    // }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    // transfer writes must occurr during the pipeline transfer stage
    commandBuffer.pipelineBarrier(sourceStage,      // which pipeline stages should occurr before barrier
                                  destinationStage, // pipeline stage in which operations will wait on the barrier
                                  {}, {}, nullptr, barrier);
}

float star::StarTextures::Texture::SelectAnisotropyLevel(const vk::PhysicalDeviceProperties &deviceProperties)
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

vk::Filter star::StarTextures::Texture::SelectTextureFiltering(const vk::PhysicalDeviceProperties &deviceProperties)
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

star::StarTextures::Texture::Texture(const Texture &other)
    : memoryResources(other.memoryResources), device(other.device), baseFormat(other.baseFormat),
      mipmapLevels(other.mipmapLevels)
{
}

star::StarTextures::Texture::~Texture()
{
}

vk::ImageView star::StarTextures::Texture::getImageView(const vk::Format *requestedFormat) const
{
    if (requestedFormat != nullptr)
    {
        // make sure the image view actually exists for the requested format
        assert(this->memoryResources->views.find(*requestedFormat) != this->memoryResources->views.end() &&
               "The image must be created with the proper image view before being requested");
        return this->memoryResources->views.at(*requestedFormat);
    }

    // just grab the first one
    for (auto &view : this->memoryResources->views)
    {
        return view.second;
    }

    throw std::runtime_error("Failed to find an image view");
}

star::StarTextures::Texture::Texture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                                     const std::string &allocationName,
                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat, const vk::SamplerCreateInfo &samplerInfo)
    : device(device), memoryResources(CreateResource(device, baseFormat, allocator, allocationCreateInfo, createInfo,
                                                     allocationName, imageViewInfos, samplerInfo)),
      baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
}

star::StarTextures::Texture::Texture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                                     const std::string &allocationName,
                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat)
    : device(device), memoryResources(CreateResource(device, baseFormat, allocator, allocationCreateInfo, createInfo,
                                                     allocationName, imageViewInfos)),
      baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
}

star::StarTextures::Texture::Texture(vk::Device &device, vk::Image &vulkanImage,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat, const vk::SamplerCreateInfo &samplerInfo)
    : device(device), baseFormat(baseFormat),
      memoryResources(std::make_shared<StarTextures::Resources>(device, vulkanImage,
                                                                CreateImageViews(device, vulkanImage, imageViewInfos),
                                                                CreateImageSampler(device, samplerInfo))),
      mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
}

star::StarTextures::Texture::Texture(vk::Device &device, vk::Image &vulkanImage,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat)
    : device(device), memoryResources(std::make_shared<StarTextures::Resources>(
                          device, vulkanImage, CreateImageViews(device, vulkanImage, imageViewInfos))),
      baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos.at(0)))
{
}

vk::Sampler star::StarTextures::Texture::CreateImageSampler(vk::Device &device,
                                                            const vk::SamplerCreateInfo &samplerCreateInfo)
{
    vk::Sampler sampler = device.createSampler(samplerCreateInfo);

    if (!sampler)
    {
        std::cerr << "Failed to create sampler" << std::endl;
        throw std::runtime_error("Failed to create sampler");
    }

    return sampler;
}

void star::StarTextures::Texture::CreateAllocation(vk::Device &device, const vk::Format &baseFormat,
                                                   VmaAllocator &allocator,
                                                   const VmaAllocationCreateInfo &allocationCreateInfo,
                                                   const vk::ImageCreateInfo &imageCreateInfo,
                                                   VmaAllocation &allocation, vk::Image &textureImage,
                                                   const std::string &allocationName)
{
    vk::ImageCreateInfo addFormat = vk::ImageCreateInfo(imageCreateInfo).setFormat(baseFormat);

    auto result = vmaCreateImage(allocator, (VkImageCreateInfo *)&addFormat, &allocationCreateInfo,
                                 (VkImage *)&textureImage, &allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image: " + result);
    }

    std::string fullAllocationName = std::string(allocationName) + "_TEXTURE";
    vmaSetAllocationName(allocator, allocation, fullAllocationName.c_str());
}

std::unordered_map<vk::Format, vk::ImageView> star::StarTextures::Texture::CreateImageViews(
    vk::Device &device, vk::Image vulkanImage, std::vector<vk::ImageViewCreateInfo> imageCreateInfos)
{
    assert(imageCreateInfos.size() > 0 && "An image view is required for every image");

    std::unordered_map<vk::Format, vk::ImageView> nViews = std::unordered_map<vk::Format, vk::ImageView>();

    for (auto &info : imageCreateInfos)
    {
        assert(nViews.find(info.format) == nViews.end() &&
               "Duplicate format requested for view. Only one view per format is supported");

        vk::Format nFormat = info.format;
        info.image = vulkanImage;

        vk::ImageView nView = CreateImageView(device, info);

        nViews.insert(std::make_pair(nFormat, nView));
    }

    return nViews;
}

void star::StarTextures::Texture::VerifyImageCreateInfo(const vk::ImageCreateInfo &createInfo)
{
    assert(createInfo.extent.width != 0 && "width cannot be 0");
    assert(createInfo.extent.height != 0 && "height cannot be 0");
    assert(createInfo.extent.depth != 0 && "depth cannot be 0");
    assert(createInfo.mipLevels != 0 && "mip levels cannnot be 0");
    assert(createInfo.arrayLayers != 0 && "array layers cannot be 0");
    assert(createInfo.initialLayout == vk::ImageLayout::eUndefined ||
           createInfo.initialLayout == vk::ImageLayout::ePreinitialized &&
               "Invalid initial layout according to vulkan specs");
    assert(createInfo.sharingMode == vk::SharingMode::eExclusive ||
           (createInfo.sharingMode == vk::SharingMode::eConcurrent && createInfo.queueFamilyIndexCount > 1) &&
               "Must have indices defined for shared resource");
}

std::shared_ptr<star::StarTextures::Resources> star::StarTextures::Texture::CreateResource(
    vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
    const VmaAllocationCreateInfo &allocationCreateInfo, const vk::ImageCreateInfo &imageCreateInfo,
    const std::string &allocationName)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image;
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    return std::make_shared<StarTextures::AllocatedResources>(device, image, allocator, allocation);
}

std::shared_ptr<star::StarTextures::Resources> star::StarTextures::Texture::CreateResource(
    vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
    const VmaAllocationCreateInfo &allocationCreateInfo, const vk::ImageCreateInfo &imageCreateInfo,
    const std::string &allocationName, const std::vector<vk::ImageViewCreateInfo> &imageViewCreateInfos)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image;
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    const auto views = CreateImageViews(device, image, imageViewCreateInfos);

    return std::make_shared<StarTextures::AllocatedResources>(device, image, views, allocator, allocation);
}

std::shared_ptr<star::StarTextures::Resources> star::StarTextures::Texture::CreateResource(
    vk::Device &device, const vk::Format &baseFormat, VmaAllocator &allocator,
    const VmaAllocationCreateInfo &allocationCreateInfo, const vk::ImageCreateInfo &imageCreateInfo,
    const std::string &allocationName, const std::vector<vk::ImageViewCreateInfo> &imageViewCreateInfos,
    const vk::SamplerCreateInfo &samplerCreateInfo)
{
    VerifyImageCreateInfo(imageCreateInfo);

    vk::Image image;
    VmaAllocation allocation;

    CreateAllocation(device, baseFormat, allocator, allocationCreateInfo, imageCreateInfo, allocation, image,
                     allocationName);

    const auto views = CreateImageViews(device, image, imageViewCreateInfos);

    const auto sampler = CreateImageSampler(device, samplerCreateInfo);

    return std::make_shared<StarTextures::AllocatedResources>(device, image, views, sampler, allocator, allocation);
}

vk::ImageView star::StarTextures::Texture::CreateImageView(vk::Device &device,
                                                           const vk::ImageViewCreateInfo &imageCreateInfo)
{
    vk::ImageView imageView = device.createImageView(imageCreateInfo);

    if (!imageView)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

uint32_t star::StarTextures::Texture::ExtractMipmapLevels(const vk::ImageViewCreateInfo &imageViewInfo)
{
    return imageViewInfo.subresourceRange.levelCount;
}

star::StarTextures::Texture::Builder::Builder(vk::Device &device, const vk::Image &vulkanImage)
    : device(device), vulkanImage(vulkanImage)
{
}

star::StarTextures::Texture::Builder::Builder(vk::Device &device, VmaAllocator &allocator)
    : device(device), createNewAllocationInfo{std::make_optional<Creators>(allocator)}
{
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setCreateInfo(
    const VmaAllocationCreateInfo &nAllocInfo, const vk::ImageCreateInfo &nCreateInfo, const std::string &nAllocName)
{
    assert(this->createNewAllocationInfo.has_value() && "Must be a builder for a new allocation");

    this->createNewAllocationInfo.value().createInfo = nCreateInfo;
    this->createNewAllocationInfo.value().allocationCreateInfo = nAllocInfo;
    this->createNewAllocationInfo.value().allocationName = nAllocName;
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::addViewInfo(
    const vk::ImageViewCreateInfo &nCreateInfo)
{
    this->viewInfos.push_back(nCreateInfo);
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setSamplerInfo(
    const vk::SamplerCreateInfo &nCreateInfo)
{
    this->samplerInfo = nCreateInfo;
    return *this;
}

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setBaseFormat(const vk::Format &nFormat)
{
    this->format = nFormat;
    return *this;
}

std::unique_ptr<star::StarTextures::Texture> star::StarTextures::Texture::Builder::build()
{
    assert(this->format.has_value());

    if (this->createNewAllocationInfo.has_value())
    {
        if (this->samplerInfo.has_value())
        {
            return std::unique_ptr<Texture>(new Texture(this->device, this->createNewAllocationInfo.value().allocator,
                           this->createNewAllocationInfo.value().createInfo,
                           this->createNewAllocationInfo.value().allocationName,
                           this->createNewAllocationInfo.value().allocationCreateInfo, this->viewInfos,
                           this->format.value(), this->samplerInfo.value()));
        }
        else
        {
            return std::unique_ptr<Texture>(new Texture(
                this->device, this->createNewAllocationInfo.value().allocator,
                this->createNewAllocationInfo.value().createInfo, this->createNewAllocationInfo.value().allocationName,
                this->createNewAllocationInfo.value().allocationCreateInfo, this->viewInfos, this->format.value()));
        }
    }
    else if (this->vulkanImage.has_value())
    {
        if (this->samplerInfo.has_value())
        {
            return std::unique_ptr<Texture>(new Texture(this->device, this->vulkanImage.value(), this->viewInfos, this->format.value(),
                           this->samplerInfo.value()));
        }
        else
        {
            return std::unique_ptr<Texture>(new Texture(this->device, this->vulkanImage.value(), this->viewInfos, this->format.value()));
        }
    }
    else
    {
        std::cerr << "Invalid builder config" << std::endl;
        throw std::runtime_error("Invalid builder config");
    }
}