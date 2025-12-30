#include "starlight/service/FrameInFlightControllerService.hpp"

namespace star::service
{
FrameInFlightControllerService::FrameInFlightControllerService()
    : star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>(*this)
{
}

FrameInFlightControllerService::FrameInFlightControllerService(FrameInFlightControllerService &&other)
    : star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>{*this},
      m_deviceEventBus{other.m_deviceEventBus}, m_deviceFrameTracker{other.m_deviceFrameTracker}
{
    if (m_deviceEventBus != nullptr)
    {
        other.cleanup(*m_deviceEventBus);
        initListeners(*m_deviceEventBus);
    }
}

FrameInFlightControllerService &FrameInFlightControllerService::operator=(FrameInFlightControllerService &&other)
{
    if (this != &other)
    {
        m_deviceEventBus = other.m_deviceEventBus;
        m_deviceFrameTracker = other.m_deviceFrameTracker;

        if (m_deviceEventBus != nullptr)
        {
            other.cleanup(*m_deviceEventBus);
            initListeners(*m_deviceEventBus);
        }
    }

    return *this;
}

void FrameInFlightControllerService::init(const uint8_t &numFramesInFlight)
{
    assert(m_deviceEventBus != nullptr); 

    initListeners(*m_deviceEventBus);
}

void FrameInFlightControllerService::setInitParameters(star::service::InitParameters &params)
{
    m_deviceEventBus = &params.eventBus;
    m_deviceFrameTracker = &params.flightTracker;
}

void FrameInFlightControllerService::shutdown()
{
    assert(m_deviceEventBus != nullptr); 

    cleanup(*m_deviceEventBus);
}

void FrameInFlightControllerService::prepForNextFrame(common::FrameTracker *frameTracker)
{
    frameTracker->getCurrent().setFrameInFlightIndex(incrementNextFrameInFlight(*frameTracker));
    frameTracker->triggerIncrementForCurrentFrame();
}

void FrameInFlightControllerService::cleanup(common::EventBus &eventBus)
{
    star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>::cleanup(eventBus);
}

void FrameInFlightControllerService::initListeners(common::EventBus &eventBus)
{
    star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>::init(eventBus);
}

uint8_t FrameInFlightControllerService::incrementNextFrameInFlight(
    const common::FrameTracker &frameTracker) const noexcept
{
    const uint8_t &max = frameTracker.getSetup().getNumFramesInFlight();
    const uint8_t &current = frameTracker.getCurrent().getFrameInFlightIndex();

    return (current + 1) % max;
}
} // namespace star::service