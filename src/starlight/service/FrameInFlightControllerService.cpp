#include "starlight/service/FrameInFlightControllerService.hpp"

namespace star::service
{
FrameInFlightControllerService::FrameInFlightControllerService() : m_frameTracker(), m_getFT(*this), m_EOF(*this)
{
}

FrameInFlightControllerService::FrameInFlightControllerService(FrameInFlightControllerService &&other)
    : m_frameTracker(std::move(other.m_frameTracker)), m_getFT(*this), m_EOF(*this),

      m_deviceEventBus{other.m_deviceEventBus}, m_deviceCmdBus{other.m_deviceCmdBus}
{
    if (m_deviceEventBus != nullptr)
    {
        other.cleanup(*m_deviceEventBus, *m_deviceCmdBus);
        initListeners(*m_deviceEventBus);
    }
}

FrameInFlightControllerService &FrameInFlightControllerService::operator=(FrameInFlightControllerService &&other)
{
    if (this != &other)
    {
        m_frameTracker = std::move(other.m_frameTracker);
        m_deviceEventBus = other.m_deviceEventBus;
        m_deviceCmdBus = other.m_deviceCmdBus;

        if (m_deviceEventBus != nullptr)
        {
            other.cleanup(*m_deviceEventBus, *m_deviceCmdBus);
            initListeners(*m_deviceEventBus);
        }
    }

    return *this;
}

void FrameInFlightControllerService::init()
{
    assert(m_deviceEventBus != nullptr);

    initListeners(*m_deviceEventBus);
}

void FrameInFlightControllerService::setInitParameters(star::service::InitParameters &params)
{
    m_deviceEventBus = &params.eventBus;
    m_deviceCmdBus = &params.commandBus;

    m_frameTracker = common::FrameTracker(params.flightTrackerSetup);
    initListeners(params.commandBus);
}

void FrameInFlightControllerService::initListeners(core::CommandBus &cmdBus)
{
    m_getFT.init(cmdBus);
}

void FrameInFlightControllerService::cleanupListeners(core::CommandBus &cmdBus)
{
    m_getFT.cleanup(cmdBus);
}

void FrameInFlightControllerService::onFrameComplete(const event::FrameComplete &event, bool &keepAlive)
{
    // update the previous count of the last frame
    m_frameTracker.triggerIncrementForCurrentFrame();

    const uint8_t nextIndex = incrementNextFrameInFlight(m_frameTracker);
    m_frameTracker.getCurrent().setFrameInFlightIndex(nextIndex);
    m_frameTracker.getCurrent().setFinalTargetImageIndex(nextIndex);
    keepAlive = true;
}

void FrameInFlightControllerService::shutdown()
{
    assert(m_deviceEventBus != nullptr);
    assert(m_deviceCmdBus != nullptr);

    cleanup(*m_deviceEventBus, *m_deviceCmdBus);
}

void FrameInFlightControllerService::onGetFrameTracker(frames::GetFrameTracker &evt) const
{
    evt.getReply().set(&m_frameTracker);
}

void FrameInFlightControllerService::cleanup(common::EventBus &eventBus, core::CommandBus &cmdBus)
{
    m_EOF.cleanup(eventBus);

    cleanupListeners(cmdBus);
}

void FrameInFlightControllerService::initListeners(common::EventBus &eventBus)
{
    m_EOF.init(eventBus);
}

uint8_t FrameInFlightControllerService::incrementNextFrameInFlight(
    const common::FrameTracker &frameTracker) const noexcept
{
    const uint8_t &max = frameTracker.getSetup().getNumFramesInFlight();
    const uint8_t &current = frameTracker.getCurrent().getFrameInFlightIndex();

    return (current + 1) % max;
}
} // namespace star::service