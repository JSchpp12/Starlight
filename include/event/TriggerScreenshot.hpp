#pragma once

#include "StarTextures/Texture.hpp"

#include <starlight/common/IEvent.hpp>

#include <array>
#include <cassert>
#include <cstring>
#include <string>


namespace star::event
{
class TriggerScreenshot : public star::common::IEvent
{
  public:
    TriggerScreenshot(StarTextures::Texture targetTexture, const char *inputName)
        : m_targetTexture(std::move(targetTexture))
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

    StarTextures::Texture getTexture()
    {
        return m_targetTexture;
    }

  private:
    static constexpr std::size_t NameSize = 25;
    StarTextures::Texture m_targetTexture;
    std::array<unsigned char, NameSize> m_screenshotName;
};
} // namespace star::event