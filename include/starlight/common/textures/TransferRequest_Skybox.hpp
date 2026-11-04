#pragma once

#include "TransferRequest_Texture.hpp"

#include "starlight/structs/SkyboxInfo.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace star::TransferRequest
{
class Skybox : public Texture
{
  public:
    Skybox(std::shared_ptr<std::future<SkyboxInfo>> skyboxInfo, uint32_t graphicsQueueFamilyIndex);
    virtual ~Skybox() = default;

    virtual void prep() override;

    virtual std::string getDebugDescription() const override;

    virtual std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(core::device::StarDevice &device) const override;

    virtual std::unique_ptr<StarTextures::Texture> createFinal(
        core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(StarBuffers::Buffer &stagingBuffer) const override;

  private:
    std::shared_ptr<std::future<SkyboxInfo>> m_skyboxInfo;
    uint32_t m_graphicsQueueFamilyIndex = 0;
    std::optional<SkyboxInfo> m_info = std::nullopt;
};
} // namespace star::TransferRequest
