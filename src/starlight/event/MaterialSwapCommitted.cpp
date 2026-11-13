#include "starlight/event/MaterialSwapCommitted.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
MaterialSwapCommitted::MaterialSwapCommitted(std::span<const MaterialSwapCommitInfo> results)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(material_swap_committed::GetUniqueTypeName())),
      m_results(results)
{
}
} // namespace star::event
