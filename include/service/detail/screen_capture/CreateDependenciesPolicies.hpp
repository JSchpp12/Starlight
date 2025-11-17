#pragma once

#include "CalleeRenderDependencies.hpp"
#include "DeviceInfo.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"

#include <vector>

namespace star::service::detail::screen_capture
{
struct DefaultCreatePolicy
{
    virtual std::vector<StarTextures::Texture> createTransferDstTextures(core::device::StarDevice &device,
                                                                         const uint8_t &numFramesInFlight,
                                                                         const vk::Extent2D &renderingResolution,
                                                                         const vk::Format &targetImageBaseFormat);

    virtual std::vector<StarBuffers::Buffer> createHostVisibleBuffers(core::device::StarDevice &device,
                                                                      const uint8_t &numFramesInFlight,
                                                                      const vk::Extent2D &renderingResolution,
                                                                      const vk::DeviceSize &bufferSize);

    CalleeRenderDependencies create(DeviceInfo &deviceInfo, const StarTextures::Texture &targetTexture,
                                    const Handle &commandBufferContainingTarget,
                                    const Handle &targetTextureReadySemaphore, const uint8_t &numFramesInFlight);
};
} // namespace star::service::detail::screen_capture