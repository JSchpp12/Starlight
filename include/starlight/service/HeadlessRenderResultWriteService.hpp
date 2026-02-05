#pragma once

#include "starlight/command/headless_render_result_write/GetFileNameForFrame.hpp"
#include "starlight/core/renderer/RendererBase.hpp"
#include "starlight/policy/ListenForRegisterMainGraphicsRendererPolicy.hpp"
#include "starlight/policy/ListenForRenderReadyForFinalization.hpp"
#include "starlight/policy/ListenForStartOfNextFramePolicy.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/service/InitParameters.hpp"

#include <star_common/EventBus.hpp>

namespace star::service
{
template <typename T>
using ListenForGetFileNameForFrame = star::policy::ListenFor<
    T, headless_render_result_write::GetFileNameForFrame,
    headless_render_result_write::get_file_name_for_frame::GetFileNameForFrameTypeName,
    &T::onGetFileNameForFrame>;

class HeadlessRenderResultWriteService
    : private star::policy::ListenForRegisterMainGraphicsRenderPolicy<HeadlessRenderResultWriteService>
{
  public:
    HeadlessRenderResultWriteService();
    HeadlessRenderResultWriteService(const HeadlessRenderResultWriteService &) = delete;
    HeadlessRenderResultWriteService &operator=(const HeadlessRenderResultWriteService &) = delete;
    HeadlessRenderResultWriteService(HeadlessRenderResultWriteService &&other);
    HeadlessRenderResultWriteService &operator=(HeadlessRenderResultWriteService &&other);
    ~HeadlessRenderResultWriteService();

    void init();

    void negotiateWorkers(star::core::WorkerPool &pool, star::job::TaskManager &tm)
    {
    }

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void cleanup(common::EventBus &eventBus);

    void onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive);

    void onRegisterMainGraphics(const event::RegisterMainGraphicsRenderer &event, bool &keepAlive);

    void onRenderReadyForFinalization(const event::RenderReadyForFinalization &event, bool &keepAlive);

    void onGetFileNameForFrame(headless_render_result_write::GetFileNameForFrame &event) const;

  private:
    friend class star::policy::ListenForRegisterMainGraphicsRenderPolicy<HeadlessRenderResultWriteService>;

    star::policy::ListenForRenderReadyForFinalization<HeadlessRenderResultWriteService> m_renderReady;
    star::policy::ListenForStartOfNextFramePolicy<HeadlessRenderResultWriteService> m_triggerCapturePolicy;
    ListenForGetFileNameForFrame<HeadlessRenderResultWriteService> m_listenForGetFileNamePolicy;
    std::vector<Handle> m_screenshotRegistrations;

    common::EventBus *m_eventBus = nullptr;
    core::CommandBus *m_cmdBus = nullptr;
    common::FrameTracker *m_frameTracker = nullptr;
    core::device::manager::ManagerCommandBuffer *m_managerCommandBuffer = nullptr;
    core::device::manager::GraphicsContainer *m_managerGraphicsContainer = nullptr;
    const core::renderer::RendererBase *m_mainGraphicsRenderer = nullptr;

    void initListeners(common::EventBus &eventBus);

    void initListeners(core::CommandBus &commandBus);

    void cleanupListeners(common::EventBus &eventBus);

    void cleanupListeners(core::CommandBus &commandBus);

    std::string getFileName(const common::FrameTracker &ft) const;
};
} // namespace star::service