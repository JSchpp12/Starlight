#pragma once

#include "starlight/core/WorkerPool.hpp"
#include "starlight/policy/ListenForPrepForNextFramePolicy.hpp"
#include "starlight/service/InitParameters.hpp"

#include <star_common/EventBus.hpp>

namespace star::service
{
class FrameInFlightControllerService
    : private star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>
{
  public:
    FrameInFlightControllerService();
    FrameInFlightControllerService(const FrameInFlightControllerService &) = delete;
    FrameInFlightControllerService &operator=(const FrameInFlightControllerService &) = delete;
    FrameInFlightControllerService(FrameInFlightControllerService &&other);
    FrameInFlightControllerService &operator=(FrameInFlightControllerService &&other);

    void init();

    void negotiateWorkers(star::core::WorkerPool &pool, job::TaskManager &tm)
    {
    }

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void cleanup(common::EventBus &eventBus);

  protected:
    void onPrepForNextFrame(const event::PrepForNextFrame &event, bool &keepAlive);

  private:
    friend class star::policy::ListenForPrepForNextFramePolicy<FrameInFlightControllerService>;
    common::EventBus *m_deviceEventBus = nullptr;
    common::FrameTracker *m_deviceFrameTracker = nullptr;

    void initListeners(common::EventBus &eventBus);

    uint8_t incrementNextFrameInFlight(const common::FrameTracker &frameTracker) const noexcept;
};
} // namespace star::service