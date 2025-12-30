#pragma once

#include "starlight/event/policy/ListenForFrameCompletePolicy.hpp"
#include "starlight/service/HeadlessRenderResultWriteService.hpp"
#include "starlight/service/InitParameters.hpp"
#include <star_common/EventBus.hpp>

namespace star::service
{
class HeadlessRenderResultWriteService : star::event::ListenForFrameCompletePolicy<HeadlessRenderResultWriteService>
{
  public:
    HeadlessRenderResultWriteService();
    HeadlessRenderResultWriteService(const HeadlessRenderResultWriteService &) = delete;
    HeadlessRenderResultWriteService &operator=(const HeadlessRenderResultWriteService &) = delete;
    HeadlessRenderResultWriteService(HeadlessRenderResultWriteService &&other); 
    HeadlessRenderResultWriteService &operator=(HeadlessRenderResultWriteService &&other); 
    ~HeadlessRenderResultWriteService(); 

    void init(const uint8_t &numFramesInFlight); 

    void setInitParameters(star::service::InitParameters &params); 

    void shutdown(); 

    void cleanup(common::EventBus &eventBus); 

  private:
    friend class star::event::ListenForFrameCompletePolicy<HeadlessRenderResultWriteService>; 
    common::EventBus *m_eventBus = nullptr;

    void initListeners(common::EventBus &eventBus); 

    void onFrameComplete(); 
};
} // namespace star::service