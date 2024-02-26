#include "RuntimeUpdateTexture.hpp"

void star::RuntimeUpdateTexture::prepRender(StarDevice& device)
{
    this->StarTexture::prepRender(device); 

    this->device = &device; 
}

void star::RuntimeUpdateTexture::updateGPU()
{
    assert(this->device != nullptr && "Before requesting GPU update, the texture needs to be prepared"); 

    vk::DeviceSize imageSize = width * height *4;

    //transfer image to writable layout
    transitionImageLayout(*this->device, this->textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal);

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
    stagingBuffer.unmap();

    device->copyBufferToImage(stagingBuffer.getBuffer(), textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    transitionImageLayout(*this->device, this->textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}
