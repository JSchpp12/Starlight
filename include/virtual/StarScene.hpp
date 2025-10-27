#pragma once

#include "Handle.hpp"
#include "Light.hpp"
#include "StarObject.hpp"
#include "StarWindow.hpp"
#include "core/renderer/SwapChainRenderer.hpp"

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace star
{
/// <summary>
/// Container for all objects in a scene.
/// </summary>
class StarScene
{
  public:
    StarScene(const Handle &deviceID, const uint8_t &numFramesInFlight,
              std::shared_ptr<StarCamera> camera,
              std::shared_ptr<core::renderer::SwapChainRenderer> presentationRenderer);

    StarScene(const Handle &deviceID, const uint8_t &numFramesInFlight,
              std::shared_ptr<StarCamera> camera,
              std::shared_ptr<core::renderer::SwapChainRenderer> presentationRenderer,
              std::vector<std::shared_ptr<core::renderer::RendererBase>> additionalRenderers);

    /// Function called every frame
    void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    void cleanupRender(core::device::DeviceContext &context)
    {
        m_presentationRenderer->cleanupRender(context);

        for (auto &renderer : m_additionalRenderers)
        {
            renderer->cleanupRender(context);
        }
    }

    std::shared_ptr<core::renderer::SwapChainRenderer> getPresentationRenderer()
    {
        return m_presentationRenderer;
    }

    std::shared_ptr<StarCamera> getCamera()
    {
        return this->m_camera;
    }

  protected:
    std::shared_ptr<star::core::renderer::SwapChainRenderer> m_presentationRenderer;
    std::vector<std::shared_ptr<star::core::renderer::RendererBase>> m_additionalRenderers;
    std::shared_ptr<StarCamera> m_camera = std::shared_ptr<StarCamera>();
};
} // namespace star