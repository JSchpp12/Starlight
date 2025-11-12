#pragma once

#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"

#include <vector>

namespace star::service::detail::screenshot
{
struct CreateDependenciesPolicies
{
    virtual ~CreateDependenciesPolicies() = default;
    virtual std::vector<StarTextures::Texture> createTransferDstTextures(core::device::StarDevice &device,
                                                                         const uint8_t &numFramesInFlight,
                                                                         const vk::Extent2D &renderingResolution,
                                                                         const vk::Format &targetImageBaseFormat) = 0;

    virtual std::vector<StarBuffers::Buffer> createHostVisibleBuffers(core::device::StarDevice &device,
                                                                      const uint8_t &numFramesInFlight,
                                                                      const vk::Extent2D &renderingResolution,
                                                                      const vk::DeviceSize &bufferSize) = 0;
};

struct DefaultCreatePolicy : public CreateDependenciesPolicies
{
    virtual ~DefaultCreatePolicy() = default;
    virtual std::vector<StarTextures::Texture> createTransferDstTextures(
        core::device::StarDevice &device, const uint8_t &numFramesInFlight, const vk::Extent2D &renderingResolution,
        const vk::Format &targetImageBaseFormat) override;

    virtual std::vector<StarBuffers::Buffer> createHostVisibleBuffers(core::device::StarDevice &device,
                                                                      const uint8_t &numFramesInFlight,
                                                                      const vk::Extent2D &renderingResolution,
                                                                      const vk::DeviceSize &bufferSize) override;
};
} // namespace star::service::detail::screenshot