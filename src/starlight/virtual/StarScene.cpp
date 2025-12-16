#include "StarScene.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"

star::StarScene::StarScene(const Handle &deviceID, const uint8_t &numFramesInFlight, std::shared_ptr<StarCamera> camera,
                           std::vector<common::Renderer> renderers)
    : m_renderers(std::move(renderers)),
      m_camera(camera)
{
}

void star::StarScene::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    for (size_t i = 0; i < m_renderers.size(); i++)
    {
        m_renderers[i].frameUpdate(context, frameInFlightIndex);
    }
}

void star::StarScene::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    for (auto &addRender : m_renderers)
    {
        addRender.prepRender(context, numFramesInFlight);
    }
}
