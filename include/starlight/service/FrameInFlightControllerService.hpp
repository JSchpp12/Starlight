#pragma once

#include "starlight/command/frames/GetFrameTracker.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/event/FrameComplete.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/policy/event/ListenFor.hpp"
#include "starlight/service/InitParameters.hpp"

#include <star_common/EventBus.hpp>

namespace star::service
{

template <typename T>
using ListenForGetFrameTracker =
    star::policy::command::ListenFor<T, frames::GetFrameTracker, frames::get_frame_tracker::GetUniqueTypeName,
                                     &T::onGetFrameTracker>;
template <typename T>
using ListenForEndOfFrame =
    star::policy::event::ListenFor<T, star::event::FrameComplete, star::event::FrameComplete::GetUniqueTypeName,
                                   &T::onFrameComplete>;

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
    ListenForGetFrameTracker<FrameInFlightControllerService> m_getFT;
    ListenForEndOfFrame<FrameInFlightControllerService> m_EOF; 

    common::EventBus *m_deviceEventBus = nullptr;
    star::core::CommandBus *m_deviceCmdBus = nullptr;

    void initListeners(common::EventBus &eventBus);

    void initListeners(core::CommandBus &cmdBus);

    void cleanupListeners(core::CommandBus &cmdBus);

    uint8_t incrementNextFrameInFlight(const common::FrameTracker &frameTracker) const noexcept;
};
} // namespace star::service