#include "starlight/policy/EngineExitAfterNumberOfFrames.hpp"

bool star::policy::EngineExitAfterNumberOfFrames::shouldExit()
{
    m_frameCounter++;
    return m_frameCounter > m_maxNumFrames;
}