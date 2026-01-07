#include "starlight/core/waiter/sync_renderer/SyncTargetRenderer.hpp"

namespace star::core::waiter::sync_renderer
{
void SyncTargetRenderer::registerListener(star::common::EventBus &eventBus)
{
    std::shared_ptr<SyncTargetRenderer> sharedThis = shared_from_this();

    eventBus.subscribe(
        common::HandleTypeRegistry::instance().registerType(event::GetStartOfNextFrameTypeName),
        common::SubscriberCallbackInfo{
            [sharedThis](const star::common::IEvent &e, bool &keepAlive) {
                sharedThis->m_startOfFrameListener.eventCallback(e, keepAlive);
            },
            [sharedThis]() -> Handle * { return sharedThis->m_startOfFrameListener.getHandleForEventBus(); },
            [sharedThis](const Handle &noLongerNeededHandle) {
                sharedThis->m_startOfFrameListener.notificationFromEventBusOfDeletion(noLongerNeededHandle);
            }});
}

void SyncTargetRenderer::onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive)
{
    if (m_createdOnFrameCount < event.getFrameTracker().getCurrent().getGlobalFrameCounter() &&
        event.getFrameTracker().getCurrent().getFrameInFlightIndex() == m_targetFrameIndex)
    {
        registerWaitWithManager();

        keepAlive = false;
    }
    else
    {
        keepAlive = true;
    }
}
} // namespace star::core::waiter::sync_renderer