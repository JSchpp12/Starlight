#pragma once

#include <starlight/common/Handle.hpp>
#include <starlight/common/IEvent.hpp>

#include <starlight/common/HandleTypeRegistry.hpp>

namespace star::event
{
constexpr std::string_view GetEnginePhaseCompleteInitTypeName = "star::event::EnginePhaseComplete::init";
constexpr std::string_view GetEnginePhaseCompleteLoadTypeName = "star::event::EnginePhaseComplete::load";

enum class Phase
{
    init,
    load // called after all of the prepRender functions are complete
};
class EnginePhaseComplete : public common::IEvent
{
  public:
    EnginePhaseComplete(Phase phase, std::string_view enginePhaseName)
        : common::IEvent(common::HandleTypeRegistry::instance().getTypeGuaranteedExist(enginePhaseName)), m_phase(phase)
    {
    }
    virtual ~EnginePhaseComplete() = default;

    const Phase &getPhase() const
    {
        return m_phase;
    }

    Phase &getPhase()
    {
        return m_phase;
    }

  private:
    Phase m_phase;
};
} // namespace star::event