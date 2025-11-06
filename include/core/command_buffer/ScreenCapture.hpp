#pragma once

#include "CommandBufferBase.hpp"
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

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

  private:
    std::vector<StarTextures::Texture> m_targetTextures;
    std::vector<StarTextures::Texture> m_transferDstTextures;
    virtual Handle registerCommandBuffer(core::device::DeviceContext &context,
                                         const uint8_t &numFramesInFlight) override;

    std::vector<StarTextures::Texture> createTransferDstTextures(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) const;
};
} // namespace star::core::command_buffer