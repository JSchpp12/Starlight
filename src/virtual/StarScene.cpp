#include "StarScene.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"

star::StarScene::StarScene(const core::device::DeviceID &deviceID, const uint8_t &numFramesInFlight,
                           std::shared_ptr<StarCamera> camera,
                           std::shared_ptr<core::renderer::SwapChainRenderer> presentationRenderer)
    : m_presentationRenderer(presentationRenderer), m_camera(camera)
{
    // m_presentationRenderer->init(globalInfoBuffers); 
}

star::StarScene::StarScene(const core::device::DeviceID &deviceID, const uint8_t &numFramesInFlight,
                           std::shared_ptr<StarCamera> camera,
                           std::shared_ptr<core::renderer::SwapChainRenderer> presentationRenderer,
                           std::vector<std::shared_ptr<core::renderer::Renderer>> renderers)
    : m_presentationRenderer(presentationRenderer), m_additionalRenderers(renderers), m_camera(camera)
{
    // m_presentationRenderer->init(globalInfoBuffers); 
}