#pragma once

#include "CommandBufferBase.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

namespace star::core::command_buffer
{
class ScreenCapture : public CommandBufferBase
{
  public:
    ScreenCapture() = default;
    ScreenCapture(std::vector<StarTextures::Texture> targetTextures) : m_targetTextures(std::move(targetTextures))
    {
    }
    ScreenCapture(const ScreenCapture &) = default;
    ScreenCapture &operator=(const ScreenCapture &) = default;
    ScreenCapture(ScreenCapture &&) = default;
    ScreenCapture &operator=(ScreenCapture &&) = default;

    virtual ~ScreenCapture() = default;

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                    const vk::Extent2D &renderingResolution);

    void cleanupRender(core::device::DeviceContext &context) override;

  private:
    std::vector<StarTextures::Texture> m_targetTextures, m_transferDstTextures;
    std::vector<StarBuffers::Buffer> m_hostVisibleBuffers;

    virtual Handle registerCommandBuffer(core::device::DeviceContext &context,
                                         const uint8_t &numFramesInFlight) override;

    std::vector<StarTextures::Texture> createTransferDstTextures(core::device::DeviceContext &context,
                                                                 const uint8_t &numFramesInFlight,
                                                                 const vk::Extent2D &renderingResolution) const;

    std::vector<StarBuffers::Buffer> createHostVisibleBuffers(core::device::DeviceContext &context,
                                                              const uint8_t &numFramesInFlight,
                                                              const vk::Extent2D &renderingResolution,
                                                              const vk::DeviceSize &size) const;

    void cleanupBuffers(core::device::DeviceContext &context);

    void cleanupIntermediateImages(core::device::DeviceContext &context);
};
} // namespace star::core::command_buffer