#include "renderer/HeadlessRenderPhaseProvider.hpp"

#include "starlight/core/renderer/HeadlessRenderPhase.hpp"
#include "starlight/core/renderer/RenderPhaseHelpers.hpp"

namespace star::core::renderer
{
HeadlessRenderPhaseProvider::HeadlessRenderPhaseProvider(core::device::DeviceContext &context,
                                                         std::shared_ptr<std::vector<Light>> lights,
                                                         std::shared_ptr<StarCamera> camera,
                                                         std::vector<std::shared_ptr<StarObject>> objects,
                                                         vk::PipelineStageFlags waitStage)
    : DefaultRenderPhaseProvider(context, std::move(lights), std::move(camera), std::move(objects))
{
    m_config.waitStage = waitStage;
}

HeadlessRenderPhaseProvider::HeadlessRenderPhaseProvider(core::device::DeviceContext &context,
                                                         std::vector<std::shared_ptr<StarObject>> objects,
                                                         std::shared_ptr<FrameData> frameData,
                                                         vk::PipelineStageFlags waitStage)
    : DefaultRenderPhaseProvider(context, std::move(objects), std::move(frameData))
{
    m_config.waitStage = waitStage;
}

std::unique_ptr<RenderPhase> HeadlessRenderPhaseProvider::build(core::device::DeviceContext &context,
                                                                RenderPhaseRegistry & /*phases*/)
{
    auto phase = std::make_unique<HeadlessRenderPhase>(context.getCmdBus(), context.getDevice().getVulkanDevice());

    DefaultRenderPhase::Builder(context)
        .setObjects(std::move(m_objects))
        .setFrameData(m_frameData)
        .setOwnsFrameData(m_createdFrameData)
        .setConfig(m_config)
        .buildInto(*phase);

    prepareHeadlessPhase(phase.get(), context);

    return phase;
}

void HeadlessRenderPhaseProvider::prepareHeadlessPhase(HeadlessRenderPhase *phase, core::device::DeviceContext &context)
{
    phase->m_imgMgr = &context.getGraphicsManagers().imageManager;

    phase->m_prepScheme.resize(context.frameTracker().getSetup().getNumFramesInFlight(), pre_pass::DoNothing{});
    phase->m_postScheme.resize(context.frameTracker().getSetup().getNumFramesInFlight(), post_pass::DoNothing{});

    phase->m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.frameTracker());
}
} // namespace star::core::renderer
