#pragma once

#include "StarTextures/Texture.hpp"

#include <starlight/common/Handle.hpp>
#include <starlight/common/IEvent.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>

#include <array>
#include <cassert>
#include <cstring>
#include <string>

namespace star::event
{
constexpr std::string TriggerScreenshotTypeName()
{
    return "star::event::trigger_screenshot";
}

class TriggerScreenshot : public star::common::IEvent
{
  public:
    TriggerScreenshot(StarTextures::Texture targetTexture, Handle &targetTextureReadySemaphore,
                      Handle &targetCommandBuffer, const char *inputName)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(TriggerScreenshotTypeName())),
          m_targetTexture(std::move(targetTexture)), m_targetTextureReadySemaphore(&targetTextureReadySemaphore),
          m_targetCommandBuffer(&targetCommandBuffer)
    {
        std::memset(m_screenshotName.data(), 0, NameSize); // zero out the array
        std::size_t len = std::min(std::strlen(inputName), NameSize - 1);
        std::memcpy(m_screenshotName.data(), inputName, len);
    }

    virtual ~TriggerScreenshot() = default;

    std::string getName() const
    {
        return {m_screenshotName.begin(), m_screenshotName.begin() + NameSize};
    }

    StarTextures::Texture getTexture() const
    {
        return m_targetTexture;
    }

    Handle *getTargetTextureReadySemaphore() const
    {
        return m_targetTextureReadySemaphore;
    }

    Handle *getTargetCommandBuffer() const
    {
        return m_targetCommandBuffer;
    }

  private:
    static constexpr std::size_t NameSize = 25;
    StarTextures::Texture m_targetTexture;
    vk::Semaphore m_textureReadyForCopy;
    Handle *m_targetTextureReadySemaphore = nullptr;
    Handle *m_targetCommandBuffer = nullptr;
    std::array<unsigned char, NameSize> m_screenshotName;
};
} // namespace star::event