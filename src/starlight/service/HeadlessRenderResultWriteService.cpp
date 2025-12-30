#include "starlight/service/HeadlessRenderResultWriteService.hpp"

#include <cassert>

using Listen = star::event::ListenForFrameCompletePolicy<star::service::HeadlessRenderResultWriteService>;

star::service::HeadlessRenderResultWriteService::HeadlessRenderResultWriteService() : Listen(*this)
{
}

star::service::HeadlessRenderResultWriteService::HeadlessRenderResultWriteService(
    HeadlessRenderResultWriteService &&other)
    : Listen(*this), m_eventBus{other.m_eventBus}
{
    if (m_eventBus != nullptr)
    {
        other.cleanup(*m_eventBus);
        initListeners(*m_eventBus);
    }
}

star::service::HeadlessRenderResultWriteService &star::service::HeadlessRenderResultWriteService::operator=(
    HeadlessRenderResultWriteService &&other)
{
    if (this != &other)
    {
        m_eventBus = other.m_eventBus;
        if (m_eventBus != nullptr)
        {
            other.cleanup(*m_eventBus);
            initListeners(*m_eventBus);
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
        Listen::cleanup(*m_eventBus);
    }
}

void star::service::HeadlessRenderResultWriteService::cleanup(common::EventBus &eventBus)
{
    Listen::cleanup(eventBus);
}

void star::service::HeadlessRenderResultWriteService::init(const uint8_t &numFramesInFlight)
{
    assert(m_eventBus != nullptr);

    initListeners(*m_eventBus);
}

void star::service::HeadlessRenderResultWriteService::initListeners(common::EventBus &eventBus)
{
    Listen::init(eventBus);
}

void star::service::HeadlessRenderResultWriteService::onFrameComplete()
{
    // todo grab the final renderer somehow and get the images from it to dispatch to the screen capture service
    std::cout << "I am offscreen here";
 }

void star::service::HeadlessRenderResultWriteService::setInitParameters(star::service::InitParameters &params)
{
    m_eventBus = &params.eventBus;
}