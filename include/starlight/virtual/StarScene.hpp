#pragma once

#include "StarCamera.hpp"
#include "starlight/object/StarObject.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/Renderer.hpp>

#include "core/renderer/IRenderPhaseProvider.hpp"

#include <functional>
#include <queue>
#include <vector>

namespace star
{
/// <summary>
/// Container for all objects in a scene.
/// </summary>
class StarScene
{
  public:
    using IsReadyFunction = std::function<bool(core::device::DeviceContext &)>;

    StarScene(IsReadyFunction isReady, std::shared_ptr<StarCamera> camera, common::Renderer primaryRenderer);
    StarScene(IsReadyFunction isReady, std::shared_ptr<StarCamera> camera, common::Renderer primaryRenderer,
              std::vector<common::Renderer> renderers);
    ~StarScene() = default;

    /// Function called every frame
    void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRender(core::device::DeviceContext &context, const common::FrameTracker::Setup &renderImageSetup);

    void cleanupRender(core::device::DeviceContext &context);

    /// Queue a render phase provider to be built at prepRender time.
    void addProvider(std::unique_ptr<core::renderer::IRenderPhaseProvider> provider);

    bool isReady(core::device::DeviceContext &context);

    std::shared_ptr<StarCamera> getCamera()
    {
        return this->m_camera;
    }

    common::Renderer &getPrimaryRenderer()
    {
        return m_primaryRenderer;
    }
    const common::Renderer &getPrimaryRenderer() const
    {
        return m_primaryRenderer;
    }

  protected:
    IsReadyFunction m_isReady;
    std::shared_ptr<StarCamera> m_camera;
    common::Renderer m_primaryRenderer;
    std::vector<common::Renderer> m_renderers;

    std::queue<std::unique_ptr<core::renderer::IRenderPhaseProvider>> m_providers;
    std::vector<std::unique_ptr<core::renderer::RenderPhase>> m_phases;
};

namespace star_scene
{
inline auto makeAlwaysReadyPolicy() -> StarScene::IsReadyFunction
{
    return [](core::device::DeviceContext &context) -> bool { return true; };
}

inline auto makeWaitForAllObjectsReadyPolicy(std::vector<std::shared_ptr<star::StarObject>> objects)
    -> StarScene::IsReadyFunction
{
    return [objects](core::device::DeviceContext &context) -> bool {
        for (size_t i{0}; i < objects.size(); i++)
        {
            if (!objects[i]->isRenderReady(context))
            {
                return false;
            }
        }
        return true;
    };
}
} // namespace star_scene
} // namespace star