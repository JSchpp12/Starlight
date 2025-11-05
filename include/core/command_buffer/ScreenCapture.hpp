#pragma once

#include "CommandBufferBase.hpp"
#include "Handle.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

namespace star::core::command_buffer
{
class ScreenCapture : public CommandBufferBase
{
  public:
    ScreenCapture(std::vector<StarTextures::Texture> targetTextures) : m_targetTextures(std::move(targetTextures))
    {
    }
    ScreenCapture(const ScreenCapture &) noexcept = delete;
    ScreenCapture &operator=(const ScreenCapture &) noexcept = delete;
    ScreenCapture(ScreenCapture &&) = default;
    ScreenCapture &operator=(ScreenCapture &&) = default;

    virtual ~ScreenCapture() = default;

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

  private:
    std::vector<StarTextures::Texture> m_targetTextures;
    virtual Handle registerCommandBuffer(core::device::DeviceContext &context,
                                         const uint8_t &numFramesInFlight) override;
};
} // namespace star::core::command_buffer