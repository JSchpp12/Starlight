#pragma once

#include "Light.hpp"
#include "StarObject.hpp"
#include "StarCamera.hpp"

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
    StarScene(const Handle &deviceID, const uint8_t &numFramesInFlight,
              std::shared_ptr<StarCamera> camera,
              std::vector<common::Renderer> renderers);

    /// Function called every frame
    void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    void cleanupRender(core::device::DeviceContext &context)
    {
        for (auto &renderer : m_renderers)
        {
            renderer.cleanupRender(context);
        }
    }

    std::shared_ptr<StarCamera> getCamera()
    {
        return this->m_camera;
    }

  protected:
    std::vector<common::Renderer> m_renderers;
    std::shared_ptr<StarCamera> m_camera = std::shared_ptr<StarCamera>();
};
} // namespace star