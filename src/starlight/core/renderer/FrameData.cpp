#include "core/renderer/FrameData.hpp"

#include "ManagerController_RenderResource_Buffer.hpp"
#include "core/device/DeviceContext.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

#include <cassert>

namespace star::core::renderer
{
FrameData &FrameData::add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller)
{
    m_controllers.push_back(std::move(controller));
    return *this;
}

void FrameData::prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight)
{
    for (auto &c : m_controllers)
        c->prepRender(context, numFramesInFlight);
    m_pending.reserve(m_controllers.size());
}

FrameData::FrameUpdateResult FrameData::frameUpdate(core::device::DeviceContext &context,
                                                    std::optional<core::graphics::SemaphoreInfo> priorSync)
{
    const uint8_t fi = context.frameTracker().getCurrent().getFrameInFlightIndex();
    // note to self: does not actually cause any memory allocation since reservable size usually remains available
    m_pending.clear();

    for (auto &c : m_controllers)
    {
        const auto [submitted, semaphore] = c->submitUpdateIfNeeded(context, fi, priorSync);
        if (submitted)
        {
            m_pending.push_back(PendingWait{.handle = c->getHandle(fi),
                                            .signalValue = semaphore->signalValue,
                                            .semaphore = semaphore->vkSemaphore,
                                            .waitStage = c->waitStage()});
        }
    }

    return FrameUpdateResult{std::span<const PendingWait>{m_pending}};
}

std::shared_ptr<ManagerController::RenderResource::Buffer> FrameData::controllerAt(size_t i) const
{
    assert(i < m_controllers.size());
    return m_controllers[i];
}
} // namespace star::core::renderer