#pragma once

#include "StarTextures/Texture.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/IEvent.hpp>

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
    TriggerScreenshot(StarTextures::Texture targetTexture, std::string screenshotName, Handle &targetCommandBuffer,
                      Handle &calleeRegistration, Handle &targetTextureReadySemaphore)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(TriggerScreenshotTypeName())),
          m_targetTexture(std::move(targetTexture)), m_screenshotName(std::move(screenshotName)),
          m_targetCommandBuffer(targetCommandBuffer), m_calleeRegistration(calleeRegistration),
          m_targetTextureReadySemaphore(&targetTextureReadySemaphore)
    {
    }
    TriggerScreenshot(StarTextures::Texture targetTexture, Handle &targetCommandBuffer, Handle &calleeRegistration,
                      std::string screenshotName)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(TriggerScreenshotTypeName())),
          m_targetTexture(std::move(targetTexture)), m_screenshotName(std::move(screenshotName)),
          m_targetCommandBuffer(targetCommandBuffer), m_calleeRegistration(calleeRegistration),
          m_targetTextureReadySemaphore(nullptr)
    {
    }
    virtual ~TriggerScreenshot() = default;

    StarTextures::Texture getTexture() const
    {
        return m_targetTexture;
    }
    Handle *getTargetTextureReadySemaphore() const
    {
        return m_targetTextureReadySemaphore;
    }
    Handle &getTargetCommandBuffer() const
    {
        return m_targetCommandBuffer;
    }
    Handle &getCalleeRegistration() const
    {
        return m_calleeRegistration;
    }
    const std::string &getName() const
    {
        return m_screenshotName;
    }

  private:
    StarTextures::Texture m_targetTexture;
    std::string m_screenshotName;
    Handle &m_targetCommandBuffer;
    Handle &m_calleeRegistration;
    Handle *m_targetTextureReadySemaphore = nullptr;
};
} // namespace star::event