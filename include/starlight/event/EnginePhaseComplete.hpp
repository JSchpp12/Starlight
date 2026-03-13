#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IEvent.hpp>

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
namespace engine_phase_complete
{
inline constexpr const char *GetUniqueTypeName()
{
    return "EvtEpc";
}
} // namespace engine_phase_complete


class EnginePhaseComplete : public common::IEvent
{
  public:
    enum class Phase
    {
        load,
        frame
    };

    EnginePhaseComplete(Phase phase, std::string_view enginePhaseName)
        : common::IEvent(common::HandleTypeRegistry::instance().getTypeGuaranteedExist(enginePhaseName)), m_phase(phase)
    {
    }
    static constexpr std::string_view GetUniqueTypeName()
    {
        return engine_phase_complete::GetUniqueTypeName();
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