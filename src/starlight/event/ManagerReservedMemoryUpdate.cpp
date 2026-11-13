#include "event/ManagerReservedMemoryUpdate.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
ManagerReservedMemoryUpdate::ManagerReservedMemoryUpdate(uint16_t managerHandleType, void *manager)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetUniqueTypeName())),
      m_managerHandleType(managerHandleType), m_manager(manager)
{
}
} // namespace star::event
