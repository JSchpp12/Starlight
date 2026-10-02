#pragma once

#include "StarMaterial.hpp"

#include <star_common/Handle.hpp>

#include <memory>

namespace star
{
class SharedCompressedTexture;

class TextureMaterial : public StarMaterial
{
  public:
    TextureMaterial(std::string texturePath);

    /// Construct with a texture that has already been loaded and transcoded.
    /// preloadTexture will use it directly instead of loading the path lazily.
    TextureMaterial(std::string texturePath, std::unique_ptr<SharedCompressedTexture> preTranscodedTexture);

    TextureMaterial(std::string texturePath, const glm::vec4 &surfaceColor, const glm::vec4 &highlightColor,
                    const glm::vec4 &ambient, const glm::vec4 &diffuse, const glm::vec4 &specular, const int &shiny);

    virtual ~TextureMaterial();

    void preloadTexture(core::device::DeviceContext &context);

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder, star::Handle commandBuffer) override;

    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        const int &numFramesInFlight) const override;

    virtual void addDescriptorSetLayoutsTo(StarDescriptorSetLayout::Builder &builder) const override;

  protected:
    std::string m_texturePath = "";
    Handle m_textureHandle = Handle();
    std::unique_ptr<SharedCompressedTexture> m_preTranscodedTexture = nullptr;

    /// Register the texture's GPU transfer-completion semaphore as a one-time
    /// wait on the provided render command buffer.
    static void registerTextureTransferWait(core::device::DeviceContext &context, star::Handle commandBuffer,
                                            star::Handle textureHandle);

    virtual std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context,
                                                            const uint8_t &numFramesInFlight,
                                                            StarShaderInfo::Builder builder) override;

  private:
};
} // namespace star