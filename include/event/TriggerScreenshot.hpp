#pragma once

#include "StarTextures/Texture.hpp"

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>
#include <starlight/common/IEvent.hpp>

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
                      Handle &targetCommandBuffer, Handle &calleeRegistration, std::string screenshotName)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(TriggerScreenshotTypeName())),
          m_targetTexture(std::move(targetTexture)), m_targetTextureReadySemaphore(targetTextureReadySemaphore),
          m_targetCommandBuffer(targetCommandBuffer), m_calleeRegistration(calleeRegistration),
          m_screenshotName(std::move(screenshotName))
    {
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

    Handle &getTargetTextureReadySemaphore() const
    {
        return m_targetTextureReadySemaphore;
    }

    Handle &getTargetCommandBuffer() const
    {
        return m_targetCommandBuffer;
    }

    Handle &getCalleeRegistration() const{
        return m_calleeRegistration;
    }

  private:
    static constexpr std::size_t NameSize = 25;
    StarTextures::Texture m_targetTexture;
    vk::Semaphore m_textureReadyForCopy;
    Handle &m_targetTextureReadySemaphore;
    Handle &m_targetCommandBuffer;
    Handle &m_calleeRegistration;
    std::string m_screenshotName;
};
} // namespace star::event