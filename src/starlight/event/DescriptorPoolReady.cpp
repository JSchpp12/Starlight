#include "starlight/event/DescriptorPoolReady.hpp"

#include <star_common/HandleTypeRegistry.hpp>

star::event::DescriptorPoolReady::DescriptorPoolReady()
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetUniqueTypeName()))
{
}