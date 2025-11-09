#pragma once

#include "Handle.hpp"
#include "core/device/DeviceContext.hpp"
#include "wrappers/graphics/StarBuffers/Buffer.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"


namespace star::core::service
{
class ScreenCapture
{
  public:
    ScreenCapture() = default;
    ScreenCapture(std::vector<StarTextures::Texture> targetTextures, std::vector<Handle> textureReadySemaphore)
        : m_targetTextures(std::move(targetTextures)),
          m_targetTexturesReadySemaphores(std::move(m_targetTexturesReadySemaphores))
    {
    }
    ScreenCapture(const ScreenCapture &) = default;
    ScreenCapture &operator=(const ScreenCapture &) = default;
    ScreenCapture(ScreenCapture &&) = default;
    ScreenCapture &operator=(ScreenCapture &&) = default;

    virtual ~ScreenCapture() = default;

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                    const vk::Extent2D &renderingResolution);

    void cleanupRender(core::device::DeviceContext &context);

  private:
    std::vector<StarTextures::Texture> m_targetTextures;
    std::vector<Handle> m_targetTexturesReadySemaphores;
    std::vector<StarTextures::Texture> m_transferDstTextures;
    std::vector<StarBuffers::Buffer> m_hostVisibleBuffers;

    virtual Handle registerCommandBuffer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    std::vector<StarTextures::Texture> createTransferDstTextures(core::device::DeviceContext &context,
                                                                 const uint8_t &numFramesInFlight,
                                                                 const vk::Extent2D &renderingResolution) const;

    std::vector<StarBuffers::Buffer> createHostVisibleBuffers(core::device::DeviceContext &context,
                                                              const uint8_t &numFramesInFlight,
                                                              const vk::Extent2D &renderingResolution,
                                                              const vk::DeviceSize &size) const;

    void cleanupBuffers(core::device::DeviceContext &context);

    void cleanupIntermediateImages(core::device::DeviceContext &context);

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                             const uint64_t &frameIndex);

    void addMemoryDependencies(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                               const uint64_t &frameIndex) const;
};
} // namespace star::core::service