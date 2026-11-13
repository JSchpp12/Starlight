#include "starlight/event/MaterialSwapRolledBack.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
MaterialSwapRolledBack::MaterialSwapRolledBack(Handle material, Handle previousResource)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetUniqueTypeName())),
      m_material(std::move(material)), m_previousResource(std::move(previousResource))
{
}
} // namespace star::event
