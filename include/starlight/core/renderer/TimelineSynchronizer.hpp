#pragma once

#include <starlight/core/device/DeviceContext.hpp>

#include <star_common/Handle.hpp>

#include <vector>

namespace star::core::renderer
{
class TimelineSynchronizer
{
    std::vector<Handle> m_timelineSemaphores;

  public:
    TimelineSynchronizer() = default;
    explicit TimelineSynchronizer(const size_t &numSyncSlotsToCreate)
    {
        m_timelineSemaphores.resize(numSyncSlotsToCreate);
    }

    void prepRender(core::device::DeviceContext &ctx);

    const std::vector<Handle> &getSemaphores() const
    {
        return m_timelineSemaphores;
    }
};
} // namespace star::core::renderer