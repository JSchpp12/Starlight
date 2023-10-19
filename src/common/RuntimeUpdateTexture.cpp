#include "RuntimeUpdateTexture.hpp"

void star::RuntimeUpdateTexture::updateGPU()
{
    vk::DeviceSize imageSize = width * height * 4;

    //transfer image to writable layout
    //transitionImageLayout(this->textureImage)
    transitionImageLayout(this->textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal);

    //create staging buffer
    StarBuffer stagingBuffer(
        *this->device,
        imageSize,
        1,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stagingBuffer.map();
    std::unique_ptr<unsigned char> textureData(this->data());
    stagingBuffer.writeToBuffer(textureData.get(), imageSize);

    device->copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    transitionImageLayout(this->textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

}
