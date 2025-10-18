#include "StarScene.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"

star::StarScene::StarScene(const core::device::DeviceID &deviceID, const uint8_t &numFramesInFlight,
                           std::shared_ptr<StarCamera> camera,
                           std::shared_ptr<core::renderer::SwapChainRenderer> presentationRenderer)
    : m_presentationRenderer(presentationRenderer), m_camera(camera)
{
}

star::StarScene::StarScene(const core::device::DeviceID &deviceID, const uint8_t &numFramesInFlight,
                           std::shared_ptr<StarCamera> camera,
                           std::shared_ptr<core::renderer::SwapChainRenderer> presentationRenderer,
                           std::vector<std::shared_ptr<core::renderer::Renderer>> renderers)
    : m_presentationRenderer(presentationRenderer), m_additionalRenderers(renderers), m_camera(camera)
{
}

void star::StarScene::frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex){
    m_presentationRenderer->frameUpdate(context, frameInFlightIndex); 

    for (size_t i = 0; i < m_additionalRenderers.size(); i++){
        m_additionalRenderers[i]->frameUpdate(context, frameInFlightIndex); 
    }
}

void star::StarScene::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    m_presentationRenderer->prepRender(context, m_presentationRenderer->getMainExtent(), numFramesInFlight); 

    for (auto &addRender : m_additionalRenderers){
        addRender->prepRender(context, m_presentationRenderer->getMainExtent(), numFramesInFlight); 
    }
}
