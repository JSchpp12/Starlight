#pragma once

#include "SharedCompressedTexture.hpp"
#include "StarTextures/Texture.hpp"
#include "TransferRequest_Texture.hpp"

#include <ktx.h>

#include <memory>
#include <vector>

namespace star::TransferRequest
{
class CompressedTextureFile : public TransferRequest::Texture
{
  public:
    CompressedTextureFile(uint32_t graphicsQueueFamilyIndex, vk::PhysicalDeviceProperties deviceProperties,
                          std::unique_ptr<SharedCompressedTexture> compressedTexture)
        : graphicsQueueFamilyIndex(std::move(graphicsQueueFamilyIndex)), deviceProperties(std::move(deviceProperties)),
          compressedTexture(std::move(compressedTexture)) {};

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                     VmaAllocator &allocator) const override;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(StarBuffers::Buffer &stagingBuffer) const override;

    static bool IsFileCompressedTexture(const std::string &filePath);

  private:
    uint32_t graphicsQueueFamilyIndex;
    vk::PhysicalDeviceProperties deviceProperties;
    std::unique_ptr<SharedCompressedTexture> compressedTexture = nullptr;

    static void getTextureInfo(const std::string &imagePath, int &width, int &height, int &channels);
};
} // namespace star::TransferRequest