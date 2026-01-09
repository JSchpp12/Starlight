#include "StarTextures/Texture.hpp"

#include "ConfigFile.hpp"
#include "StarTextures/AllocatedResources.hpp"
#include "logging/LoggingFactory.hpp"

#include <cassert>
#include <cmath>
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
    : memoryResources(CreateResource(device, baseFormat, allocator, allocationCreateInfo, createInfo, allocationName,
                                     imageViewInfos, samplerInfo)),
      baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos)), baseExtent(createInfo.extent),
      size(CalculateSize(baseFormat, createInfo.extent, createInfo.arrayLayers, createInfo.imageType, mipmapLevels))
{
}

star::StarTextures::Texture::Texture(vk::Device &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                                     const std::string &allocationName,
                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat)
    : memoryResources(CreateResource(device, baseFormat, allocator, allocationCreateInfo, createInfo, allocationName,
                                     imageViewInfos)),
      baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos)), baseExtent(createInfo.extent),
      size(CalculateSize(baseFormat, createInfo.extent, createInfo.arrayLayers, createInfo.imageType, mipmapLevels))
{
}

star::StarTextures::Texture::Texture(vk::Device &device, vk::Image &vulkanImage,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat, const vk::SamplerCreateInfo &samplerInfo,
                                     const vk::Extent3D &baseExtent, vk::DeviceSize size)
    : baseFormat(baseFormat),
      memoryResources(std::make_shared<StarTextures::Resources>(
          vulkanImage, CreateImageViews(device, vulkanImage, imageViewInfos), CreateImageSampler(device, samplerInfo))),
      mipmapLevels(ExtractMipmapLevels(imageViewInfos)), baseExtent(baseExtent), size(std::move(size))
{
}

star::StarTextures::Texture::Texture(vk::Device &device, vk::Image &vulkanImage,
                                     const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                                     const vk::Format &baseFormat, const vk::Extent3D &baseExtent, vk::DeviceSize size)
    : memoryResources(std::make_shared<StarTextures::Resources>(vulkanImage,
                                                                CreateImageViews(device, vulkanImage, imageViewInfos))),
      baseFormat(baseFormat), mipmapLevels(ExtractMipmapLevels(imageViewInfos)), baseExtent(baseExtent),
      size(std::move(size))
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

    vk::Result result;
    {
        auto tmpResult = vmaCreateImage(allocator, (VkImageCreateInfo *)&addFormat, &allocationCreateInfo,
                                        (VkImage *)&textureImage, &allocation, nullptr);
        result = static_cast<vk::Result>(tmpResult);
    }

    if (result != vk::Result::eSuccess)
    {
        LogImageCreateFailure(result);

        std::string message = "Failed to create image";
        throw std::runtime_error(message);
    }

    std::string fullAllocationName = std::string(allocationName) + "_TEXTURE";
    vmaSetAllocationName(allocator, allocation, fullAllocationName.c_str());
}

std::unordered_map<vk::Format, vk::ImageView> star::StarTextures::Texture::CreateImageViews(
    vk::Device &device, vk::Image vulkanImage, std::vector<vk::ImageViewCreateInfo> imageCreateInfos)
{
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
    assert((createInfo.initialLayout == vk::ImageLayout::eUndefined) ||
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

    return std::make_shared<StarTextures::AllocatedResources>(image, allocation, allocator);
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

    return std::make_shared<StarTextures::AllocatedResources>(image, views, allocation, allocator);
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

    return std::make_shared<StarTextures::AllocatedResources>(image, views, sampler, allocation, allocator);
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

uint32_t star::StarTextures::Texture::ExtractMipmapLevels(const std::vector<vk::ImageViewCreateInfo> &imageViewInfo)
{
    if (imageViewInfo.size() > 0)
    {
        return imageViewInfo[0].subresourceRange.levelCount;
    }
    return 1;
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

star::StarTextures::Texture::Builder &star::StarTextures::Texture::Builder::setSizeInfo(vk::DeviceSize size,
                                                                                        vk::Extent3D resolution)
{
    overrideSize = std::make_optional(size);
    overrideResolution = std::make_optional(resolution);

    return *this;
}

std::unique_ptr<star::StarTextures::Texture> star::StarTextures::Texture::Builder::buildUnique()
{
    return std::make_unique<StarTextures::Texture>(build());
}

std::shared_ptr<star::StarTextures::Texture> star::StarTextures::Texture::Builder::buildShared()
{
    return std::make_shared<StarTextures::Texture>(build());
}

star::StarTextures::Texture star::StarTextures::Texture::Builder::build()
{
    assert(this->format.has_value());

    if (this->createNewAllocationInfo.has_value())
    {
        if (this->samplerInfo.has_value())
        {
            return {this->device,
                    this->createNewAllocationInfo.value().allocator,
                    this->createNewAllocationInfo.value().createInfo,
                    this->createNewAllocationInfo.value().allocationName,
                    this->createNewAllocationInfo.value().allocationCreateInfo,
                    this->viewInfos,
                    this->format.value(),
                    this->samplerInfo.value()};
        }
        else
        {
            return {this->device,
                    this->createNewAllocationInfo.value().allocator,
                    this->createNewAllocationInfo.value().createInfo,
                    this->createNewAllocationInfo.value().allocationName,
                    this->createNewAllocationInfo.value().allocationCreateInfo,
                    this->viewInfos,
                    this->format.value()};
        }
    }
    else if (this->vulkanImage.has_value())
    {
        assert(overrideSize.has_value() && "Size MUST be calculated/provided for externally created/managed texture");
        if (this->samplerInfo.has_value())
        {
            return {this->device,         this->vulkanImage.value(), this->viewInfos,
                    this->format.value(), this->samplerInfo.value(), overrideResolution.value(),
                    overrideSize.value()};
        }
        else
        {
            return {this->device,         this->vulkanImage.value(),  this->viewInfos,
                    this->format.value(), overrideResolution.value(), overrideSize.value()};
        }
    }
    else
    {
        std::cerr << "Invalid builder config" << std::endl;
        throw std::runtime_error("Invalid builder config");
    }
}

void star::StarTextures::Texture::LogImageCreateFailure(const vk::Result &result)
{
    std::ostringstream oss;
    oss << "Failed to create image with Vulkan error: ";

    if (result == vk::Result::eErrorFeatureNotPresent){
        oss << "eErrorFeatureNotPresent"; 
    }

    core::logging::log(boost::log::trivial::error, oss.str());
}

vk::DeviceSize star::StarTextures::Texture::CalculateSize(const vk::Format &baseFormat, const vk::Extent3D &baseExtent,
                                                          const uint32_t &arrayLayers, const vk::ImageType &imageType,
                                                          const uint32_t &mipmapLevels)
{
    const FormatInfo info = FormatInfo::Create(baseFormat);
    vk::DeviceSize totalSize = 0;

    for (uint32_t i = 0; i < mipmapLevels; i++)
    {
        auto levelExtent = GetLevelExtent(baseExtent, i);

        if (imageType == vk::ImageType::e1D)
        {
            levelExtent.height = 1;
            levelExtent.width = 1;
        }
        else if (imageType == vk::ImageType::e2D)
        {
            levelExtent.depth = 1;
        }

        vk::DeviceSize levelBytes = 0;

        if (info.blockCompressed)
        {
            const uint32_t bx = CeilDiv(levelExtent.width, info.blockExtent.width);
            const uint32_t by = CeilDiv(levelExtent.height, info.blockExtent.height);
            const uint32_t bz = CeilDiv(levelExtent.depth, info.blockExtent.depth);
            levelBytes = static_cast<vk::DeviceSize>(bx) * static_cast<vk::DeviceSize>(by) *
                         static_cast<vk::DeviceSize>(bz) * static_cast<vk::DeviceSize>(info.blockBytes);
        }
        else
        {
            levelBytes =
                static_cast<vk::DeviceSize>(levelExtent.width) * static_cast<vk::DeviceSize>(levelExtent.height) *
                static_cast<vk::DeviceSize>(levelExtent.depth) * static_cast<vk::DeviceSize>(info.bytesPerPixel);
        }

        totalSize += levelBytes;
    }

    return totalSize;
}

uint32_t star::StarTextures::Texture::CeilDiv(const uint32_t &width, const uint32_t &height)
{
    return (width + height - 1) / height;
}

vk::Extent3D star::StarTextures::Texture::GetLevelExtent(const vk::Extent3D &baseExtent, const uint32_t &level)
{
    return vk::Extent3D()
        .setWidth(std::max(1u, baseExtent.width >> level))
        .setHeight(std::max(1u, baseExtent.height >> level))
        .setDepth(std::max(1u, baseExtent.depth >> level));
}