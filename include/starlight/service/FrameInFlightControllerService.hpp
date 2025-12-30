#pragma once

#include "starlight/event/policy/ListenForPrepForNextFramePolicy.hpp"
#include "starlight/service/InitParameters.hpp"
#include <star_common/EventBus.hpp>

namespace star::service
{
class FrameInFlightControllerService
    : public star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>
{
  public:
    FrameInFlightControllerService();
    FrameInFlightControllerService(const FrameInFlightControllerService &) = delete;
    FrameInFlightControllerService &operator=(const FrameInFlightControllerService &) = delete;
    FrameInFlightControllerService(FrameInFlightControllerService &&other); 
    FrameInFlightControllerService &operator=(FrameInFlightControllerService &&other); 

    void init(const uint8_t &numFramesInFlight);

    void setInitParameters(star::service::InitParameters &params); 
    
    void shutdown(); 

    void cleanup(common::EventBus &eventBus); 

  protected:
    void prepForNextFrame(common::FrameTracker *frameTracker);

  private:
    friend class star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>;
    common::EventBus *m_deviceEventBus = nullptr; 
    common::FrameTracker *m_deviceFrameTracker = nullptr;

    void initListeners(common::EventBus &eventBus); 

    uint8_t incrementNextFrameInFlight(const common::FrameTracker &frameTracker) const noexcept;
};
} // namespace star::service