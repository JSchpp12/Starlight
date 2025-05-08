#include "StarTexture.hpp"


#include "ConfigFile.hpp"

#include <cassert>
#include <stdexcept>


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
        this->device.getDevice().destroySampler(this->sampler);

    for (auto &item : this->views)
    {
        this->device.getDevice().destroyImageView(item.second);
    }

    if (this->allocator.has_value() && this->memory.has_value())
    {
        vmaDestroyImage(*this->allocator.value(), this->vulkanImage, this->memory.value());
    }
}

const vk::ImageView &star::StarTexture::getImageView(const vk::Format *requestedFormat) const
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

star::StarTexture::StarTexture(StarDevice &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                               const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                               const vk::SamplerCreateInfo &samplerInfo)
    : device(device), allocator(&allocator), memory(VmaAllocation())
{
    CreateAllocation(device.getDevice(), allocator, allocationCreateInfo, createInfo, this->memory.value(),
                     this->vulkanImage, allocationName);

    this->views = CreateImageViews(device.getDevice(), imageViewInfos);
    this->sampler = CreateImageSampler(device.getDevice(), samplerInfo);
}

star::StarTexture::StarTexture(StarDevice &device, VmaAllocator &allocator, const vk::ImageCreateInfo &createInfo,
                               const std::string &allocationName, const VmaAllocationCreateInfo &allocationCreateInfo,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos)
    : device(device), allocator(&allocator), memory(VmaAllocation())
{
    CreateAllocation(device.getDevice(), allocator, allocationCreateInfo, createInfo, this->memory.value(),
                     this->vulkanImage, allocationName);

    this->views = CreateImageViews(device.getDevice(), imageViewInfos);
}

star::StarTexture::StarTexture(StarDevice &device, vk::Image &vulkanImage,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos,
                               const vk::SamplerCreateInfo &samplerInfo)
    : device(device), vulkanImage(vulkanImage)
{
    this->views = CreateImageViews(device.getDevice(), imageViewInfos);
    this->sampler = CreateImageSampler(device.getDevice(), samplerInfo);
}

star::StarTexture::StarTexture(StarDevice &device, vk::Image &vulkanImage,
                               const std::vector<vk::ImageViewCreateInfo> &imageViewInfos)
    : device(device), vulkanImage(vulkanImage)
{
    this->views = CreateImageViews(device.getDevice(), imageViewInfos);
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
    vk::Device &device, const std::vector<vk::ImageViewCreateInfo> &imageCreateInfos)
{
    assert(imageCreateInfos.size() > 0 && "An image view is required for every image");

    std::unordered_map<vk::Format, vk::ImageView> nViews = std::unordered_map<vk::Format, vk::ImageView>();

    for (const auto &info : imageCreateInfos)
    {
        vk::Format nFormat = info.format;
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

star::StarTexture::Builder::Builder(star::StarDevice &device, const vk::Image &vulkanImage)
    : device(device), vulkanImage(vulkanImage)
{
}

star::StarTexture::Builder::Builder(StarDevice &device, VmaAllocator &allocator)
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

star::StarTexture::Builder &star::StarTexture::Builder::setViewInfo(const vk::ImageViewCreateInfo &nCreateInfo)
{
    this->viewInfos.push_back(nCreateInfo);
    return *this;
}

star::StarTexture::Builder &star::StarTexture::Builder::setSamplerInfo(const vk::SamplerCreateInfo &nCreateInfo)
{
    this->samplerInfo = nCreateInfo;
    return *this;
}

std::unique_ptr<star::StarTexture> star::StarTexture::Builder::build()
{
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
                this->createNewAllocationInfo.value().allocationCreateInfo,
                this->createNewAllocationInfo.value().allocationName, this->viewInfos, this->samplerInfo.value()));
        }
        else
        {
            return std::unique_ptr<StarTexture>(
                new StarTexture(this->device, this->createNewAllocationInfo.value().allocator,
                                this->createNewAllocationInfo.value().createInfo,
                                this->createNewAllocationInfo.value().allocationCreateInfo,
                                this->createNewAllocationInfo.value().allocationName, this->viewInfos));
        }
    }
    else if (this->vulkanImage.has_value())
    {
        if (this->samplerInfo.has_value())
        {
            return std::unique_ptr<StarTexture>(
                new StarTexture(this->device, this->vulkanImage.value(), this->viewInfos));
        }
        else
        {
            return std::unique_ptr<StarTexture>(
                new StarTexture(this->device, this->vulkanImage.value(), this->viewInfos, this->samplerInfo.value()));
        }
    }
    else
    {
        std::cerr << "Invalid builder config" << std::endl;
        throw std::runtime_error("Invalid builder config");
    }
    return nullptr;
}