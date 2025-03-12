#include "TransferRequest_TextureFile.hpp"

#include "Enums.hpp"
#include "FileHelpers.hpp"
#include "ConfigFile.hpp"

#include "stb_image.h"

#include <assert.h>

star::TransferRequest::TextureFile::TextureFile(const std::string& imagePath) : imagePath(imagePath){
    assert(star::FileHelpers::FileExists(this->imagePath) && "Provided path does not exist");
}

star::StarTexture::TextureCreateSettings star::TransferRequest::TextureFile::getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const{
    int width, height, channels = 0;
    getTextureInfo(this->imagePath, width, height, channels);

    auto iSet = star::StarTexture::TextureCreateSettings{};
    iSet.width = width;
    iSet.height = height; 
    iSet.channels = 4;      //overriding the channels to 4 for simplicity
    iSet.byteDepth = 1;
    iSet.depth = 1;
    iSet.imageUsage = vk::ImageUsageFlagBits::eSampled;
    iSet.imageFormat = vk::Format::eR8G8B8A8Srgb;
    iSet.aspectFlags = vk::ImageAspectFlagBits::eColor;
    iSet.memoryUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
    iSet.allocationCreateFlags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    iSet.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    iSet.isMutable = false; 
    iSet.createSampler = true; 
    iSet.anisotropyLevel = this->selectAnisotropyLevel(deviceProperties);
    iSet.textureFilteringMode = this->selectTextureFiltering(deviceProperties);
    return iSet;
}

void star::TransferRequest::TextureFile::writeData(star::StarBuffer& stagingBuffer) const{
    int l_width, l_height, l_channels = 0;
    unsigned char* pixelData(stbi_load(this->imagePath.c_str(), &l_width, &l_height, &l_channels, STBI_rgb_alpha));

    if (!pixelData) {
        throw std::runtime_error("Unable to load image");
    }

    //apply overriden alpha value if needed

    for (int i = 0; i < l_height; i++) {
        for (int j = 0; j < l_width; j++) {
            unsigned char* pixelOffset = pixelData + (i + l_height * j) * 4;
            pixelOffset[3] = (unsigned char)255;
        }
    }

    vk::DeviceSize imageSize = l_width * l_height * 4; 

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(pixelData, imageSize);
    stagingBuffer.unmap();

    stbi_image_free(pixelData);
}

float star::TransferRequest::TextureFile::selectAnisotropyLevel(const vk::PhysicalDeviceProperties& deviceProperties) const{
    std::string anisotropySetting = star::ConfigFile::getSetting(star::Config_Settings::texture_anisotropy);
    float anisotropyLevel = 1.0f;

    if (anisotropySetting == "max")
        anisotropyLevel = deviceProperties.limits.maxSamplerAnisotropy;
    else{
        anisotropyLevel = std::stof(anisotropySetting);
    }
    assert(anisotropyLevel >= 1.0f);

    return anisotropyLevel;
}

vk::Filter star::TransferRequest::TextureFile::selectTextureFiltering(const vk::PhysicalDeviceProperties& deviceProperties) const{
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

void star::TransferRequest::TextureFile::getTextureInfo(const std::string& imagePath, int& width, int& height, int& channels){
    stbi_info(imagePath.c_str(), &width, &height, &channels);
}