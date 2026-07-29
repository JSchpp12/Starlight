#include "StarScene.hpp"

#include "core/renderer/RenderPhase.hpp"

#include <queue>
#include <utility>

star::StarScene::StarScene(star::StarScene::IsReadyFunction isReady, std::shared_ptr<StarCamera> camera,
                           common::Renderer primaryRenderer)
    : m_isReady(std::move(isReady)), m_camera(std::move(camera)), m_primaryRenderer(std::move(primaryRenderer))
{
}

star::StarScene::StarScene(star::StarScene::IsReadyFunction isReady, std::shared_ptr<StarCamera> camera,
                           common::Renderer primaryRenderer, std::vector<common::Renderer> renderers)
    : m_isReady(std::move(isReady)), m_camera(std::move(camera)), m_primaryRenderer(std::move(primaryRenderer)),
      m_renderers(std::move(renderers))
{
}

void star::StarScene::addProvider(std::unique_ptr<star::core::renderer::IRenderPhaseProvider> provider)
{
    m_providers.push(std::move(provider));
}

bool star::StarScene::isReady(core::device::DeviceContext &context)
{
    assert(m_isReady);

    return m_isReady(context);
}

void star::StarScene::cleanupRender(core::device::DeviceContext &context)
{
    for (auto &phase : m_phases)
    {
        phase->cleanupRender(context);
    }

    for (auto &renderer : m_renderers)
    {
        renderer.cleanupRender(context);
    }

    m_primaryRenderer.cleanupRender(context);
}

void star::StarScene::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    m_camera->frameUpdate(context, frameInFlightIndex);

    for (auto &phase : m_phases)
    {
        phase->frameUpdate(context);
    }

    for (size_t i = 0; i < m_renderers.size(); i++)
    {
        m_renderers[i].frameUpdate(context);
    }

    m_primaryRenderer.frameUpdate(context);
}

void star::StarScene::prepRender(core::device::DeviceContext &context,
                                 const common::FrameTracker::Setup &renderImageSetup)
{
    while (!m_providers.empty())
    {
        auto provider = std::move(m_providers.front());
        m_providers.pop();
        m_phases.push_back(provider->build(context));
    }

    for (auto &addRender : m_renderers)
    {
        addRender.prepRender(context);
    }

    m_primaryRenderer.prepRender(context);
}
