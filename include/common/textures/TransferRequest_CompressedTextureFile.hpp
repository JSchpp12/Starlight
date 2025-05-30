#pragma once

#include "SharedCompressedTexture.hpp"
#include "StarTexture.hpp"
#include "TransferRequest_Texture.hpp"


#include <ktx.h>

#include <memory>
#include <vector>


namespace star::TransferRequest
{
class CompressedTextureFile : public TransferRequest::Texture
{
  public:
    CompressedTextureFile(const uint32_t &graphicsQueueFamilyIndex,
                          const vk::PhysicalDeviceProperties &deviceProperties,
                          std::shared_ptr<SharedCompressedTexture> compressedTexture, const uint8_t &mipMapIndex);

    virtual std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    virtual std::unique_ptr<star::StarTexture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(StarBuffer &srcBuffer, StarTexture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(StarBuffer &stagingBuffer) const override;

  private:
    std::shared_ptr<SharedCompressedTexture> compressedTexture = nullptr;
    const uint8_t mipMapIndex;
    const uint32_t graphicsQueueFamilyIndex;
    const vk::PhysicalDeviceProperties deviceProperties;

    static void getTextureInfo(const std::string &imagePath, int &width, int &height, int &channels);
};
} // namespace star::TransferRequest