#include "starlight/event/PipelineReady.hpp"

#include <star_common/HandleTypeRegistry.hpp>

star::event::PipelineReady::PipelineReady(star::Handle triggeringPipeline)
    : common::IEvent(
          common::HandleTypeRegistry::instance().registerType(star::event::pipeline_ready::GetUniqueTypeName())),
      m_triggeringPipeline(std::move(triggeringPipeline))
{
}