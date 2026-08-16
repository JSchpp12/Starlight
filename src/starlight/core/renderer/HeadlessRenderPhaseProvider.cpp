#include "renderer/HeadlessRenderPhaseProvider.hpp"

#include "starlight/core/renderer/HeadlessRenderPhase.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>

#include <star_common/HandleTypeRegistry.hpp>

namespace star::core::renderer
{
static std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
                                                  const star::common::FrameTracker &ft) noexcept
{
    const size_t num = static_cast<size_t>(ft.getSetup().getNumFramesInFlight());

    auto handles = std::vector<star::Handle>(num);
    for (size_t i{0}; i < handles.size(); i++)
    {
        void *r = nullptr;
        evtBus.emit(star::core::device::system::event::ManagerRequest(
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetSemaphoreEventTypeName),
            star::core::device::manager::SemaphoreRequest{true}, handles[i], &r));

        if (r == nullptr)
        {
            STAR_THROW("Unable to create new semaphore");
        }
    }

    return handles;
}

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
    auto phase = std::make_unique<HeadlessRenderPhase>();

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
    phase->m_cmdBus = &context.getCmdBus();
    phase->m_device = context.getDevice().getVulkanDevice();
    phase->m_imgMgr = &context.getGraphicsManagers().imageManager;

    phase->m_prepScheme.resize(context.frameTracker().getSetup().getNumFramesInFlight(), pre_pass::DoNothing{});
    phase->m_postScheme.resize(context.frameTracker().getSetup().getNumFramesInFlight(), post_pass::DoNothing{});

    phase->m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.frameTracker());
}
} // namespace star::core::renderer
