#include "starlight/policy/DefaultEngineLoopPolicy.hpp"

#include "core/logging/LoggingFactory.hpp"

namespace star::policy
{
void DefaultEngineLoopPolicy::logFrameCount() const noexcept
{
    std::ostringstream oss;
    oss << "Engine Frame: " << std::to_string(m_frameCounter);
    core::logging::info(oss.str());
}
} // namespace star::policy