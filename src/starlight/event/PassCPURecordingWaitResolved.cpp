#include "starlight/event/PassCPURecordingWaitResolved.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
PassCPURecordingWaitResolved::PassCPURecordingWaitResolved(Results results)
    : common::IEvent(
          common::HandleTypeRegistry::instance().registerType(pass_cpu_recording_wait_resolved::GetUniqueTypeName())),
      m_results(std::move(results))
{
}
} // namespace star::event
