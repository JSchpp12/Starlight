#include "StarScene.hpp"

#include "core/renderer/RenderPhase.hpp"

#include <cassert>
#include <queue>
#include <utility>

star::StarScene::StarScene(star::StarScene::IsReadyFunction isReady, std::shared_ptr<StarCamera> camera)
    : m_isReady(std::move(isReady)), m_camera(std::move(camera))
{
}

star::Handle star::StarScene::addProvider(std::unique_ptr<star::core::renderer::IRenderPhaseProvider> provider)
{
    const star::Handle handle = m_phases.reserve();
    m_providers.push({std::move(provider), handle});
    m_phaseHandles.push_back(handle);
    return handle;
}

bool star::StarScene::isReady(core::device::DeviceContext &context)
{
    assert(m_isReady);

    return m_isReady(context);
}

void star::StarScene::cleanupRender(core::device::DeviceContext &context)
{
    for (auto &renderer : m_renderers)
    {
        renderer.cleanupRender(context);
    }

    m_primaryRenderer.cleanupRender(context);

    for (auto &handle : m_phaseHandles)
    {
        auto &phase = m_phases.get(handle);
        if (phase)
            phase->cleanupRender(context);
    }
}

void star::StarScene::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    m_camera->frameUpdate(context, frameInFlightIndex);

    for (size_t i = 0; i < m_renderers.size(); i++)
    {
        m_renderers[i].frameUpdate(context);
    }

    m_primaryRenderer.frameUpdate(context);

    for (auto &handle : m_phaseHandles)
    {
        auto &phase = m_phases.get(handle);
        if (phase)
            phase->frameUpdate(context);
    }
}

void star::StarScene::prepRender(core::device::DeviceContext &context,
                                 const common::FrameTracker::Setup &renderImageSetup)
{
    for (auto &addRender : m_renderers)
    {
        addRender.prepRender(context);
    }

    m_primaryRenderer.prepRender(context);

    while (!m_providers.empty())
    {
        auto [provider, handle] = std::move(m_providers.front());
        m_providers.pop();
        m_phases.get(handle) = provider->build(context, *this);
    }
}
