#pragma once

#include "TransferRequest_Texture.hpp"

#include <string>

namespace star::TransferRequest
{
class TextureFile : public Texture
{
  public:
    TextureFile(const std::string &imagePath, const uint32_t &graphicsQueueFamilyIndex,
                const vk::PhysicalDeviceProperties &deviceProperties);

    virtual std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(StarBuffer &srcBuffer, StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(StarBuffer &stagingBuffer) const override;

  protected:
  private:
    const uint32_t graphicsQueueFamilyIndex;
    const std::string imagePath;
    const vk::PhysicalDeviceProperties deviceProperties;

    static void GetTextureInfo(const std::string &imagePath, int &width, int &height, int &channels);
};
} // namespace star::TransferRequest