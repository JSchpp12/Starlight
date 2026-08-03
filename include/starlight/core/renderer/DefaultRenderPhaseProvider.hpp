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

    virtual std::unique_ptr<RenderPhase> build(core::device::DeviceContext &context, RenderPhaseRegistry &phases) override;

  protected:
    RenderPhaseConfig m_config;
    std::vector<std::shared_ptr<StarObject>> m_objects;
    std::shared_ptr<FrameData> m_frameData;
    std::shared_ptr<ManagerController::RenderResource::Buffer> m_infoManagerLightData, m_infoManagerLightList,
        m_infoManagerCamera;
    bool ownsRenderResourceControllers = false;

    void initBuffers(core::device::DeviceContext &context, std::shared_ptr<std::vector<Light>> lights,
                     std::shared_ptr<StarCamera> camera);

    core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest(DefaultRenderPhase *phase);

    static std::vector<StarRenderGroup> CreateRenderingGroups(core::device::DeviceContext &context,
                                                              std::vector<std::shared_ptr<StarObject>> objects);

    void buildCore(DefaultRenderPhase *phase, core::device::DeviceContext &device);
    virtual RenderTargets createRenderTargets(core::device::DeviceContext &context, RenderingContext &renderingContext);
};
} // namespace star::core::renderer