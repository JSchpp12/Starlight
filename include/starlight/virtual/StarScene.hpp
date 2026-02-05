#pragma once

#include "Light.hpp"
#include "StarCamera.hpp"
#include "StarObject.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>
#include <star_common/Renderer.hpp>

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
    StarScene(std::shared_ptr<StarCamera> camera, common::Renderer primaryRenderer);
    StarScene(std::shared_ptr<StarCamera> camera, common::Renderer primaryRenderer,
              std::vector<common::Renderer> renderers);

    /// Function called every frame
    void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRender(core::device::DeviceContext &context, const common::FrameTracker::Setup &renderImageSetup);

    void cleanupRender(core::device::DeviceContext &context);

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
    std::shared_ptr<StarCamera> m_camera;
    common::Renderer m_primaryRenderer;
    std::vector<common::Renderer> m_renderers;
};
} // namespace star