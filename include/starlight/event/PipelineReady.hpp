#pragma once

#include <star_common/Handle.hpp>
#include <star_common/IEvent.hpp>

namespace star::event
{
namespace pipeline_ready
{
constexpr const char *GetUniqueTypeName()
{
    return "EvtPRdy";
}
} // namespace pipeline_ready

class PipelineReady : public common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return pipeline_ready::GetUniqueTypeName();
    }

    explicit PipelineReady(star::Handle triggeringPipeline);

    [[nodiscard]] const star::Handle &getTriggeringPipeline() const noexcept
    {
        return m_triggeringPipeline;
    }

  private:
    star::Handle m_triggeringPipeline;
};
} // namespace star::event