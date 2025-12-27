#include "StarScene.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"

star::StarScene::StarScene(std::shared_ptr<StarCamera> camera, common::Renderer primaryRenderer)
    : m_camera(std::move(camera)), m_primaryRenderer(std::move(primaryRenderer))
{
}

star::StarScene::StarScene(std::shared_ptr<StarCamera> camera, common::Renderer primaryRenderer,
                           std::vector<common::Renderer> renderers)
    : m_camera(std::move(camera)), m_primaryRenderer(std::move(primaryRenderer)), m_renderers(std::move(renderers))
{ 
}

void star::StarScene::cleanupRender(core::device::DeviceContext &context)
{
    for (auto &renderer : m_renderers)
    {
        renderer.cleanupRender(context);
    }
    
    m_primaryRenderer.cleanupRender(context);
}
void star::StarScene::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    m_camera->frameUpdate(context, frameInFlightIndex);

    for (size_t i = 0; i < m_renderers.size(); i++)
    {
        m_renderers[i].frameUpdate(context);
    }

    m_primaryRenderer.frameUpdate(context);
}

void star::StarScene::prepRender(core::device::DeviceContext &context, const common::FrameTracker::Setup &renderImageSetup)
{
    for (auto &addRender : m_renderers)
    {
        addRender.prepRender(context);
    }

    m_primaryRenderer.prepRender(context);
}
