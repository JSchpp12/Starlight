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
    TriggerScreenshot(StarTextures::Texture targetTexture, std::string screenshotPath, Handle &targetCommandBuffer,
                      Handle &calleeRegistration, vk::Semaphore targetTextureReadySemaphore)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(TriggerScreenshotTypeName())),
          m_targetTexture(std::move(targetTexture)), m_screenshotPath(std::move(screenshotPath)),
          m_targetTextureReadySemaphore(std::move(targetTextureReadySemaphore)),
          m_targetCommandBuffer(targetCommandBuffer), m_calleeRegistration(calleeRegistration)
    {
    }
    TriggerScreenshot(StarTextures::Texture targetTexture, std::string screenshotPath, Handle &targetCommandBuffer,
                      Handle &calleeRegistration)
        : common::IEvent(common::HandleTypeRegistry::instance().registerType(TriggerScreenshotTypeName())),
          m_targetTexture(std::move(targetTexture)), m_screenshotPath(std::move(screenshotPath)),
          m_targetTextureReadySemaphore(nullptr), m_targetCommandBuffer(targetCommandBuffer),
          m_calleeRegistration(calleeRegistration)
    {
    }
    virtual ~TriggerScreenshot() = default;

    const StarTextures::Texture &getTexture() const
    {
        return m_targetTexture;
    }
    const vk::Semaphore &getTargetTextureReadySemaphore() const
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
    const std::string &getPath() const
    {
        return m_screenshotPath;
    }

  private:
    StarTextures::Texture m_targetTexture;
    std::string m_screenshotPath;
    vk::Semaphore m_targetTextureReadySemaphore = VK_NULL_HANDLE;
    Handle &m_targetCommandBuffer;
    Handle &m_calleeRegistration;
};
} // namespace star::event