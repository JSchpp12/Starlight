#pragma once

#include "TransferRequest_Texture.hpp"

#include <string>

namespace star::TransferRequest
{
class TextureFile : public Texture
{
  public:
    TextureFile(uint32_t graphicsQueueFamilyIndex, vk::PhysicalDeviceProperties deviceProperties,
                std::string imagePath);

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                     VmaAllocator &allocator) const override;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(StarBuffers::Buffer &stagingBuffer) const override;

  protected:
  private:
    uint32_t graphicsQueueFamilyIndex;
    vk::PhysicalDeviceProperties deviceProperties;
    std::string m_imagePath;

    static void GetTextureInfo(const std::string &imagePath, int &width, int &height, int &channels);
};
} // namespace star::TransferRequest