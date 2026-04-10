#pragma once
#include "starlight/core/WorkerPool.hpp"
#include "starlight/event/FrameComplete.hpp"
#include "starlight/policy/command/ListenForGetFrameTracker.hpp"
#include "starlight/policy/event/ListenForFrameComplete.hpp"
#include "starlight/service/InitParameters.hpp"

#include <star_common/EventBus.hpp>

namespace star::service
{
class FrameInFlightControllerService
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

    void cleanup(common::EventBus &eventBus, core::CommandBus &cmdBus);

    void onGetFrameTracker(frames::GetFrameTracker &evt) const;

    void onFrameComplete(const event::FrameComplete &event, bool &keepAlive);

  private:
    common::FrameTracker m_frameTracker;
    star::policy::command::ListenForGetFrameTracker<FrameInFlightControllerService> m_getFT;
    star::policy::event::ListenForFrameComplete<FrameInFlightControllerService> m_EOF;

    common::EventBus *m_deviceEventBus = nullptr;
    star::core::CommandBus *m_deviceCmdBus = nullptr;

    void initListeners(common::EventBus &eventBus);

    void initListeners(core::CommandBus &cmdBus);

    void cleanupListeners(core::CommandBus &cmdBus);

    uint8_t incrementNextFrameInFlight(const common::FrameTracker &frameTracker) const noexcept;
};
} // namespace star::service