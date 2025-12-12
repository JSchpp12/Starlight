#include "StarScene.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"

star::StarScene::StarScene(const Handle &deviceID, const uint8_t &numFramesInFlight, std::shared_ptr<StarCamera> camera,
                           common::Renderer presentationRenderer)
    : m_presentationRenderer(std::move(presentationRenderer)), m_camera(camera)
{
}

star::StarScene::StarScene(const Handle &deviceID, const uint8_t &numFramesInFlight, std::shared_ptr<StarCamera> camera,
                           common::Renderer presentationRenderer, std::vector<common::Renderer> renderers)
    : m_presentationRenderer(std::move(presentationRenderer)), m_additionalRenderers(std::move(renderers)),
      m_camera(camera)
{
}

void star::StarScene::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    for (size_t i = 0; i < m_additionalRenderers.size(); i++)
    {
        m_additionalRenderers[i].frameUpdate(context, frameInFlightIndex);
    }
    m_presentationRenderer.frameUpdate(context, frameInFlightIndex);
}

void star::StarScene::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    for (auto &addRender : m_additionalRenderers)
    {
        addRender.prepRender(context, numFramesInFlight);
    }
    m_presentationRenderer.prepRender(context, numFramesInFlight);
}
