#pragma once

#include "SharedCompressedTexture.hpp"
#include "StarMaterial.hpp"

#include <star_common/Handle.hpp>

#include <starlight/wrappers/graphics/StarSemaphore.hpp>

#include <memory>

namespace star
{
class SharedCompressedTexture;

class TextureMaterial : public StarMaterial
{
  public:
    explicit TextureMaterial(std::string texturePath);

    /// Construct with a texture that has already been loaded and transcoded.
    /// preloadTexture will use it directly instead of loading the path lazily.
    TextureMaterial(std::string texturePath, std::unique_ptr<SharedCompressedTexture> preTranscodedTexture);

    /// Construct around a texture that has already been created and registered with the render resources.
    /// preloadTexture will use it directly instead of loading a path.
    explicit TextureMaterial(Handle preloadedTexture);

    TextureMaterial(std::string texturePath, glm::vec4 surfaceColor, glm::vec4 highlightColor, glm::vec4 ambient,
                    glm::vec4 diffuse, glm::vec4 specular, int shiny);

    virtual ~TextureMaterial();

    void preloadTexture(core::device::DeviceContext &context);

    virtual void prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder, star::Handle commandBuffer) override;

    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        int numFramesInFlight) const override;

    virtual void addDescriptorSetLayoutsTo(StarDescriptorSetLayout::Builder &builder) const override;

  protected:
    std::string m_texturePath = "";
    Handle m_textureHandle = Handle();
    /// Semaphore the texture's transfer signals when its GPU work retires, as reported by SubmitTextureTransfer.
    star::StarSemaphore m_textureTransfer{};
    // TODO: Remove because this can also be used for regular textures which muddies the waters here.
    std::unique_ptr<SharedCompressedTexture> m_preTranscodedTexture = nullptr;

    /// Register the texture's GPU transfer-completion semaphore as a one-time
    /// wait on the provided render command buffer. No-op when there is no transfer to wait on.
    static void registerTextureTransferWait(core::device::DeviceContext &context, star::Handle commandBuffer,
                                            star::Handle textureHandle, const star::StarSemaphore &transferSemaphore);

    virtual std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context,
                                                            uint8_t numFramesInFlight,
                                                            StarShaderInfo::Builder builder) override;
};
} // namespace star