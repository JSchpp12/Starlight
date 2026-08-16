#pragma once

#include "Light.hpp"
#include "StarCamera.hpp"
#include "core/renderer/DefaultRenderPhase.hpp"
#include "core/renderer/IRenderPhaseProvider.hpp"
#include "core/renderer/RenderPhaseConfig.hpp"
#include "core/renderer/RenderTargets.hpp"
#include "starlight/object/StarObject.hpp"

#include <star_common/FrameTracker.hpp>

#include <memory>
#include <vector>

namespace star::core::renderer
{
/// Builds a DefaultRenderPhase. Holds the setup recipe (render-target provider,
/// config) and the per-phase shared buffer controllers (via FrameData)
class DefaultRenderPhaseProvider : public IRenderPhaseProvider
{
  public:
    DefaultRenderPhaseProvider() = default;
    DefaultRenderPhaseProvider(core::device::DeviceContext &context, std::shared_ptr<std::vector<Light>> lights,
                               std::shared_ptr<StarCamera> camera, std::vector<std::shared_ptr<StarObject>> objects);

    DefaultRenderPhaseProvider(core::device::DeviceContext &context, std::vector<std::shared_ptr<StarObject>> objects,
                               std::shared_ptr<FrameData> frameData);
    virtual ~DefaultRenderPhaseProvider() = default;
    DefaultRenderPhaseProvider(const DefaultRenderPhaseProvider &) = delete;
    DefaultRenderPhaseProvider &operator=(const DefaultRenderPhaseProvider &) = delete;
    DefaultRenderPhaseProvider(DefaultRenderPhaseProvider &&) = default;
    DefaultRenderPhaseProvider &operator=(DefaultRenderPhaseProvider &&) = default;

    std::shared_ptr<FrameData> getFrameData()
    {
        return m_frameData;
    }

    virtual std::unique_ptr<RenderPhase> build(core::device::DeviceContext &context,
                                               RenderPhaseRegistry &phases) override;

  protected:
    RenderPhaseConfig m_config;
    std::vector<std::shared_ptr<StarObject>> m_objects;
    std::shared_ptr<FrameData> m_frameData;
    bool m_createdFrameData = false;
};
} // namespace star::core::renderer
