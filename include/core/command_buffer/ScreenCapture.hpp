#pragma once

#include "CommandBufferBase.hpp"
#include "Handle.hpp"
#include "wrappers/graphics/StarTextures/Texture.hpp"

namespace star::core::command_buffer
{
class ScreenCapture : public CommandBufferBase
{
  public:
    ScreenCapture(const std::vector<StarTextures::Texture> targetTextures) : m_targetTextures(std::move(targetTextures))
    {
    }
    ScreenCapture(const ScreenCapture &) noexcept = delete;
    ScreenCapture &operator=(const ScreenCapture &) noexcept = delete;
    ScreenCapture(ScreenCapture &&other) noexcept : m_targetTextures(std::move(other.m_targetTextures)) {};
    ScreenCapture &operator=(ScreenCapture &&other) noexcept
    {
        if (this != &other)
        {
            std::destroy_at(&m_targetTextures);
            std::construct_at(&m_targetTextures, other.m_targetTextures);
        }
        return *this;
    }
    virtual ~ScreenCapture() = default;

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

  private:
    const std::vector<StarTextures::Texture> m_targetTextures;
    virtual Handle registerCommandBuffer(core::device::DeviceContext &context,
                                         const uint8_t &numFramesInFlight) override;
};
} // namespace star::core::command_buffer