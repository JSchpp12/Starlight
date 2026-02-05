#include "starlight/service/HeadlessRenderResultWriteService.hpp"

#include "starlight/core/waiter/sync_renderer/Factory.hpp"
#include "starlight/event/TriggerScreenshot.hpp"

#include <cassert>

using GraphicsListen =
    star::policy::ListenForRegisterMainGraphicsRenderPolicy<star::service::HeadlessRenderResultWriteService>;

star::service::HeadlessRenderResultWriteService::HeadlessRenderResultWriteService()
    : GraphicsListen(*this), m_renderReady(*this), m_triggerCapturePolicy(*this), m_listenForGetFileNamePolicy(*this)
{
}

star::service::HeadlessRenderResultWriteService::HeadlessRenderResultWriteService(
    HeadlessRenderResultWriteService &&other)
    : GraphicsListen(*this), m_renderReady(*this), m_triggerCapturePolicy(*this), m_listenForGetFileNamePolicy(*this)
{
    m_eventBus = other.m_eventBus;
    m_cmdBus = other.m_cmdBus;

    if (m_eventBus != nullptr && m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_eventBus);
        other.cleanupListeners(*m_cmdBus);

        initListeners(*m_eventBus);
        initListeners(*m_cmdBus);
    }
}

star::service::HeadlessRenderResultWriteService &star::service::HeadlessRenderResultWriteService::operator=(
    HeadlessRenderResultWriteService &&other)
{
    if (this != &other)
    {
        m_eventBus = other.m_eventBus;
        m_cmdBus = other.m_cmdBus;

        if (m_eventBus != nullptr && m_cmdBus != nullptr)
        {
            other.cleanupListeners(*m_eventBus);
            other.cleanupListeners(*m_cmdBus);

            initListeners(*m_eventBus);
            initListeners(*m_cmdBus);
        }
    }
    return *this;
}

void star::service::HeadlessRenderResultWriteService::shutdown()
{
    assert(m_eventBus != nullptr);
    cleanup(*m_eventBus);
}

star::service::HeadlessRenderResultWriteService::~HeadlessRenderResultWriteService()
{
    if (m_eventBus != nullptr)
    {
        cleanup(*m_eventBus);
    }
}

void star::service::HeadlessRenderResultWriteService::onGetFileNameForFrame(
    headless_render_result_write::GetFileNameForFrame &event) const
{
    assert(m_frameTracker != nullptr);

    event.getReply().set(getFileName(*m_frameTracker));
}

void star::service::HeadlessRenderResultWriteService::initListeners(core::CommandBus &commandBus)
{
    m_listenForGetFileNamePolicy.init(commandBus);
}

void star::service::HeadlessRenderResultWriteService::cleanupListeners(common::EventBus &eventBus)
{
    GraphicsListen::cleanup(eventBus);
    m_renderReady.cleanup(eventBus);
    m_triggerCapturePolicy.cleanup(eventBus);
}

void star::service::HeadlessRenderResultWriteService::cleanupListeners(core::CommandBus &commandBus)
{
    m_listenForGetFileNamePolicy.cleanup(commandBus);
}

void star::service::HeadlessRenderResultWriteService::cleanup(common::EventBus &eventBus)
{
    cleanupListeners(*m_eventBus);
    cleanupListeners(*m_cmdBus);
}

void star::service::HeadlessRenderResultWriteService::init()
{
    assert(m_eventBus != nullptr && m_cmdBus != nullptr);

    initListeners(*m_eventBus);
    initListeners(*m_cmdBus);

    m_screenshotRegistrations.resize(m_frameTracker->getSetup().getNumFramesInFlight());
}

void star::service::HeadlessRenderResultWriteService::initListeners(common::EventBus &eventBus)
{
    GraphicsListen::init(eventBus);
    m_renderReady.init(eventBus);
    m_triggerCapturePolicy.init(eventBus);
}

void star::service::HeadlessRenderResultWriteService::onStartOfNextFrame(const event::StartOfNextFrame &event,
                                                                         bool &keepAlive)
{
    // todo grab the final renderer somehow and get the images from it to dispatch to the screen capture service
    assert(m_eventBus != nullptr);
    assert(m_frameTracker != nullptr);
    assert(m_managerGraphicsContainer != nullptr);
    assert(m_mainGraphicsRenderer != nullptr);
    assert(m_managerCommandBuffer != nullptr);

    const size_t index = static_cast<size_t>(m_frameTracker->getCurrent().getFrameInFlightIndex());
    vk::Semaphore semaphore = m_managerCommandBuffer->getDefault().commandBuffer->getCompleteSemaphores()[index];
    star::StarTextures::Texture targetImage =
        m_managerGraphicsContainer->imageManager.get(m_mainGraphicsRenderer->getRenderToColorImages()[index])->texture;
    auto commandBuffer = m_mainGraphicsRenderer->getCommandBuffer();

    const auto name = getFileName(*m_frameTracker);
    m_eventBus->emit(event::TriggerScreenshot{std::move(targetImage), name, commandBuffer,
                                              m_screenshotRegistrations[index], std::move(semaphore)});

    keepAlive = true;
}

void star::service::HeadlessRenderResultWriteService::setInitParameters(star::service::InitParameters &params)
{
    m_eventBus = &params.eventBus;
    m_cmdBus = &params.commandBus;
    m_frameTracker = &params.flightTracker;
    m_managerGraphicsContainer = &params.graphicsManagers;
    m_managerCommandBuffer = &params.commandBufferManager;
}

void star::service::HeadlessRenderResultWriteService::onRegisterMainGraphics(
    const event::RegisterMainGraphicsRenderer &event, bool &keepAlive)
{
    m_mainGraphicsRenderer = event.getRenderer();

    keepAlive = false;
}

void star::service::HeadlessRenderResultWriteService::onRenderReadyForFinalization(
    const event::RenderReadyForFinalization &event, bool &keepAlive)
{
    assert(m_eventBus && m_managerCommandBuffer);
    // create a waiter to update the target renderer
    core::waiter::sync_renderer::Factory(*m_eventBus, *m_managerCommandBuffer)
        .setSemaphore(event.getFinalDoneSemaphore())
        .setCreatedOnFrameCount(m_frameTracker->getCurrent().getGlobalFrameCounter())
        .setTargetFrameInFlightIndex(m_frameTracker->getCurrent().getFinalTargetImageIndex())
        .setTargetCommandBuffer(m_mainGraphicsRenderer->getCommandBuffer())
        .build();

    keepAlive = true;
}

std::string star::service::HeadlessRenderResultWriteService::getFileName(const common::FrameTracker &ft) const
{
    return "Frame - " + std::to_string(ft.getCurrent().getGlobalFrameCounter()) + ".png";
}