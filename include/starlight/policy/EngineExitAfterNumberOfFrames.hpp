#pragma once

#include <star_common/FrameTracker.hpp>
#include <stdint.h>

namespace star::policy
{
class EngineExitAfterNumberOfFrames
{
  public:
    explicit EngineExitAfterNumberOfFrames(uint64_t maxNumFrames) : m_maxNumFrames(std::move(maxNumFrames))
    {
    }

    bool shouldExit();

  private:
    uint64_t m_maxNumFrames{0};
    uint64_t m_frameCounter{0};
};
} // namespace star::policy