#include "FileTexture.hpp"
#include "FileTexture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

star::FileTexture::FileTexture(const std::string& pathToImage)
    : pathToFile(pathToImage), 
    StarTexture(TextureCreateSettings{
        getTextureWidth(pathToImage),
        getTextureHeight(pathToImage),
        4,
        1,
        1,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor,
        VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        false, true})
{

}

void star::FileTexture::saveToDisk(const std::string& path)
{
    //auto possibleData = this->data(); 
    //if (this->rawData && possibleData.has_value()) {
    //    std::unique_ptr<unsigned char>& data = possibleData.value(); 

    //    stbi_write_png(path.c_str(), this->getWidth(), this->getHeight(), this->getChannels(), data.get(), this->getWidth() * this->getChannels());
    //}
}

std::unique_ptr<star::StarBuffer> star::FileTexture::loadImageData(StarDevice& device)
{
    vk::DeviceSize imageSize = (this->creationSettings.width * this->creationSettings.height * this->creationSettings.channels * this->creationSettings.depth) * this->creationSettings.byteDepth;
    std::unique_ptr<star::StarBuffer> stagingBuffer = std::make_unique<star::StarBuffer>(
        device,
        imageSize,
        uint32_t(1),
        VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent
    );

    {

        int l_width, l_height, l_channels = 0;
        unsigned char* pixelData(stbi_load(pathToFile.c_str(), &l_width, &l_height, &l_channels, STBI_rgb_alpha));

        if (!pixelData) {
            throw std::runtime_error("Unable to load image");
        }

        //apply overriden alpha value if needed
        if (this->overrideAlpha.has_value() && this->getChannels() == 4) {
            for (int i = 0; i < l_height; i++) {
                for (int j = 0; j < l_width; j++) {
                    unsigned char* pixelOffset = pixelData + (i + this->getHeight() * j) * this->getChannels();
                    pixelOffset[3] = this->overrideAlpha.value().raw_a();
                }
            }
        }

        stagingBuffer->map();
        stagingBuffer->writeToBuffer(pixelData, imageSize);
        stagingBuffer->unmap();

        stbi_image_free(pixelData);
    }

    return stagingBuffer;
}

int star::FileTexture::getTextureHeight(const std::string& pathToFile) {
    int width, height, channels = 0; 
    stbi_info(pathToFile.c_str(), &width, &height, &channels);

    return height;
}

int star::FileTexture::getTextureWidth(const std::string& pathToFile) {
    int width, height, channels = 0; 
    stbi_info(pathToFile.c_str(), &width, &height, &channels); 

    return width; 
}

int star::FileTexture::getTextureChannels(const std::string& pathToFile) {
    int width, height, channels = 0; 
    stbi_info(pathToFile.c_str(), &width, &height, &channels);

    return channels; 
}