#pragma once

#include <star_common/IEvent.hpp>

#include <cstdint>
#include <string_view>

namespace star::event
{
namespace manager_reserved_memory_update
{
inline constexpr const char *GetUniqueTypeName()
{
    return "EvtMRMU";
}
} // namespace manager_reserved_memory_update

/// @brief Emitted after a manager's backing record storage reallocates.
/// @details Raw pointers previously obtained from the manager's record storage are invalid after this event.
/// Handles remain stable, so listeners should re-resolve their resources through the manager pointer.
/// Emission is synchronous on the inserting thread; the engine currently only mutates managers on the main
/// thread.
class ManagerReservedMemoryUpdate : public star::common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return manager_reserved_memory_update::GetUniqueTypeName();
    }

    /// @param managerHandleType Registered handle type id of the emitting manager (e.g. the Image manager's
    ///                          TResourceTypeName id).
    /// @param manager Pointer to the emitting manager, so listeners can re-resolve resources against it.
    ManagerReservedMemoryUpdate(uint16_t managerHandleType, void *manager);

    uint16_t getManagerHandleType() const noexcept
    {
        return m_managerHandleType;
    }

    void *getManager() const noexcept
    {
        return m_manager;
    }

  private:
    uint16_t m_managerHandleType;
    void *m_manager;
};
} // namespace star::event
