#pragma once

#include "starlight/core/renderer/RendererBase.hpp"
#include "starlight/policy/ListenForRegisterMainGraphicsRendererPolicy.hpp"
#include "starlight/policy/ListenForPrepForNextFramePolicy.hpp"
#include "starlight/policy/ListenForRenderReadyForFinalization.hpp"
#include "starlight/service/InitParameters.hpp"
#include <star_common/EventBus.hpp>

namespace star::service
{
class HeadlessRenderResultWriteService
    : private star::policy::ListenForPrepForNextFramePolicy<HeadlessRenderResultWriteService>,
      private star::policy::ListenForRegisterMainGraphicsRenderPolicy<HeadlessRenderResultWriteService>
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

    void onPrepForNextFrame(const event::PrepForNextFrame &eevnt, bool &keepAlive); 

    void onRegisterMainGraphics(const event::RegisterMainGraphicsRenderer &event, bool &keepAlive);

    void onRenderReadyForFinalization(const event::RenderReadyForFinalization &event, bool &keepAlive);
  private:
    friend class star::policy::ListenForPrepForNextFramePolicy<HeadlessRenderResultWriteService>;
    friend class star::policy::ListenForRegisterMainGraphicsRenderPolicy<HeadlessRenderResultWriteService>;

    star::policy::ListenForRenderReadyForFinalization<HeadlessRenderResultWriteService> m_renderReady; 
    std::vector<Handle> m_screenshotRegistrations; 
    common::EventBus *m_eventBus = nullptr;
    common::FrameTracker *m_frameTracker = nullptr;
    core::device::manager::ManagerCommandBuffer *m_managerCommandBuffer = nullptr;
    core::device::manager::GraphicsContainer *m_managerGraphicsContainer = nullptr;  
    const core::renderer::RendererBase *m_mainGraphicsRenderer = nullptr;

    void initListeners(common::EventBus &eventBus);
};
} // namespace star::service