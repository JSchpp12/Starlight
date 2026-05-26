#include "starlight/core/renderer/TimelineSynchronizer.hpp"

#include <Exceptions.hpp>
#include <cstdint>
#include <device/managers/Semaphore.hpp>
#include <device/system/event/ManagerRequest.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <utility>
#include <vector>

namespace star::core::renderer
{
static std::vector<star::Handle> CreateSemaphores(star::core::device::DeviceContext &context, const size_t &numToCreate)
{
    auto semaphores = std::vector<star::Handle>(numToCreate);

    for (size_t i{0}; i < (size_t)numToCreate; i++)
    {
        {
            auto request = core::device::manager::SemaphoreRequest(uint64_t{0});

            context.getEventBus().emit(core::device::system::event::ManagerRequest(
                common::HandleTypeRegistry::instance()
                    .getType(core::device::manager::GetSemaphoreEventTypeName)
                    .value(),
                std::move(request), semaphores[i]));
        }

        if (!semaphores[i].isInitialized())
        {
            STAR_THROW("failed to create semaphores for a frame");
        }
    }

    return semaphores;
}

void TimelineSynchronizer::prepRender(core::device::DeviceContext &ctx)
{
    m_timelineSemaphores = CreateSemaphores(ctx, m_timelineSemaphores.size());
}
} // namespace star::core::renderer
