#pragma once

#include <stdint.h>

namespace star::policy
{
class DefaultEngineLoopPolicy
{
  public:
    void frameUpdate()
    {
        m_frameCounter++; 
        logFrameCount();
    }

  private:
    uint64_t m_frameCounter = 0; 

    void logFrameCount() const noexcept; 
};
} // namespace star::core