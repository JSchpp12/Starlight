#pragma once

#include "StarCamera.hpp"
#include "starlight/object/StarObject.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/Renderer.hpp>
#include <star_common/special_types/SpecialHandleTypes.hpp>

#include "core/LinearHandleContainer.hpp"
#include "core/renderer/IRenderPhaseProvider.hpp"
#include "core/renderer/RenderPhase.hpp"

#include <functional>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

namespace star
{
/// <summary>
/// Container for all objects in a scene.
/// </summary>
class StarScene : public core::renderer::RenderPhaseRegistry
{
  public:
    using IsReadyFunction = std::function<bool(core::device::DeviceContext &)>;

    static constexpr size_t MaxRenderPhases = 8;

    StarScene(IsReadyFunction isReady, std::shared_ptr<StarCamera> camera);
    ~StarScene() = default;

    /// Function called every frame
    void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRender(core::device::DeviceContext &context, const common::FrameTracker::Setup &renderImageSetup);

    void cleanupRender(core::device::DeviceContext &context);

    /// Queue a render phase provider to be built at prepRender time. The returned
    /// handle is the slot the phase will occupy once built; thread it into any
    /// sibling provider that needs to reach this phase's data.
    Handle addProvider(std::unique_ptr<core::renderer::IRenderPhaseProvider> provider);

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

    /// RenderPhaseRegistry: look up a built phase by its handle.
    core::renderer::RenderPhase *getPhase(const Handle &handle) override
    {
        return m_phases.isFilled(handle) ? m_phases.get(handle).get() : nullptr;
    }

  protected:
    IsReadyFunction m_isReady;
    std::shared_ptr<StarCamera> m_camera;
    common::Renderer m_primaryRenderer;
    std::vector<common::Renderer> m_renderers;

    std::queue<std::pair<std::unique_ptr<core::renderer::IRenderPhaseProvider>, Handle>> m_providers;
    core::LinearHandleContainer<std::unique_ptr<core::renderer::RenderPhase>> m_phases{
        common::HandleTypeRegistry::instance().getTypeGuaranteedExist(common::special_types::RenderPhaseTypeName),
        MaxRenderPhases};
    std::vector<Handle> m_phaseHandles;
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
