#include "renderer/DefaultRenderPhaseProvider.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "starlight/command/command_order/DeclarePass.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/core/waiter/one_shot/GenericEvent.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>

#include <cassert>
#include <functional>

namespace star::core::renderer
{
static void RegisterWithCommandOrder(const star::core::CommandBus &cmdBus, star::common::EventBus &evtBus,
                                     star::core::device::manager::Queue &qm, Handle commandBuffer)
{
    auto *queue = star::core::helper::GetEngineDefaultQueue(evtBus, qm, star::Queue_Type::Tgraphics);
    assert(queue != nullptr && "Failed to acquire default engine queue");

    cmdBus.submit(star::command_order::DeclarePass{std::move(commandBuffer), queue->getParentQueueFamilyIndex()});
}

DefaultRenderPhaseProvider::DefaultRenderPhaseProvider(core::device::DeviceContext &context,
                                                       std::shared_ptr<std::vector<Light>> lights,
                                                       std::shared_ptr<StarCamera> camera,
                                                       std::vector<std::shared_ptr<StarObject>> objects)
    : ownsRenderResourceControllers(true)
{
    m_objects = std::move(objects);
    initBuffers(context, std::move(lights), camera);
}

DefaultRenderPhaseProvider::DefaultRenderPhaseProvider(core::device::DeviceContext &context,
                                                       std::vector<std::shared_ptr<StarObject>> objects,
                                                       std::shared_ptr<FrameData> frameData)
    : m_frameData(std::move(frameData)), ownsRenderResourceControllers(false)
{
    m_objects = std::move(objects);
    m_infoManagerCamera = m_frameData->controllerAt(0);
    m_infoManagerLightData = m_frameData->controllerAt(1);
    m_infoManagerLightList = m_frameData->controllerAt(2);
}

void DefaultRenderPhaseProvider::initBuffers(core::device::DeviceContext &context,
                                             std::shared_ptr<std::vector<Light>> lights,
                                             std::shared_ptr<StarCamera> camera)
{
    auto cameraController = std::make_shared<ManagerController::RenderResource::GlobalInfo>(camera);
    auto lightInfoController = std::make_shared<ManagerController::RenderResource::LightInfo>(
        context.frameTracker().getSetup().getNumFramesInFlight(), lights);
    auto lightListController = std::make_shared<ManagerController::RenderResource::LightList>(
        context.frameTracker().getSetup().getNumFramesInFlight(), lights);

    m_frameData = std::make_shared<FrameData>();
    m_frameData->add(std::move(cameraController))
        .add(std::move(lightInfoController))
        .add(std::move(lightListController));

    m_infoManagerCamera = m_frameData->controllerAt(0);
    m_infoManagerLightData = m_frameData->controllerAt(1);
    m_infoManagerLightList = m_frameData->controllerAt(2);
}

std::vector<star::StarRenderGroup> DefaultRenderPhaseProvider::CreateRenderingGroups(
    core::device::DeviceContext &context, std::vector<std::shared_ptr<StarObject>> objects)
{
    auto renderingGroups = std::vector<StarRenderGroup>();

    for (size_t i = 0; i < objects.size(); i++)
    {
        StarRenderGroup *match = nullptr;

        for (size_t j = 0; j < renderingGroups.size(); j++)
        {
            if (renderingGroups[j].isObjectCompatible(*objects[i]))
            {
                match = &renderingGroups[j];
                break;
            }
        }

        if (match != nullptr)
        {
            match->addObject(objects[i]);
        }
        else
        {
            renderingGroups.emplace_back(context, objects[i]);
        }
    }

    return renderingGroups;
}

core::device::manager::ManagerCommandBuffer::Request DefaultRenderPhaseProvider::getCommandBufferRequest(
    DefaultRenderPhase *phase)
{
    return core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&DefaultRenderPhase::recordCommandBuffer, phase, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3),
        .order = m_config.order,
        .orderIndex = m_config.orderIndex,
        .type = m_config.queueType,
        .waitStage = m_config.waitStage,
        .willBeSubmittedEachFrame = m_config.willBeSubmittedEachFrame,
        .recordOnce = m_config.recordOnce,
        .overrideBufferSubmissionCallback = phase->getSubmissionOverride(),
    };
}

std::unique_ptr<RenderPhase> DefaultRenderPhaseProvider::build(core::device::DeviceContext &device,
                                                               RenderPhaseRegistry & /*phases*/)
{
    auto phase = std::make_unique<DefaultRenderPhase>();
    buildCore(phase.get(), device);
    return phase;
}

void DefaultRenderPhaseProvider::buildCore(DefaultRenderPhase *phase, core::device::DeviceContext &c)
{
    // transfer construction state (created in this provider's ctor) to the phase
    phase->m_objects = std::move(m_objects);
    phase->m_frameData = m_frameData;
    phase->m_infoManagerLightData = m_infoManagerLightData;
    phase->m_infoManagerLightList = m_infoManagerLightList;
    phase->m_infoManagerCamera = m_infoManagerCamera;
    phase->ownsRenderResourceControllers = ownsRenderResourceControllers;

    phase->m_renderGroups = CreateRenderingGroups(c, phase->m_objects);
    phase->m_commandBuffer = c.getManagerCommandBuffer().submit(getCommandBufferRequest(phase),
                                                                c.frameTracker().getCurrent().getGlobalFrameCounter());
    RegisterWithCommandOrder(c.getCmdBus(), c.getEventBus(), c.getGraphicsManagers().queueManager,
                             phase->m_commandBuffer);

    phase->m_frameData->prepRender(c, c.frameTracker().getSetup().getNumFramesInFlight());

    phase->m_renderingContext.targetResolution = c.getEngineResolution();

    phase->m_renderTargets = createRenderTargets(c, phase->m_renderingContext);

    for (auto &group : phase->m_renderGroups)
    {
        group.prepRender(c);
    }

    // needs to wait until after prepRenderPhase ==> when descriptor pool will be created
    star::core::waiter::one_shot::GenericEvent<DefaultRenderPhase::WaitForDescriptorPoolReady,
                                               star::event::DescriptorPoolReady>::Builder(c.getEventBus())
        .setPayload(DefaultRenderPhase::WaitForDescriptorPoolReady{
            phase->getRenderTargetInfo(),
            std::bind(&DefaultRenderPhase::manualCreateDescriptors, phase, std::placeholders::_1), c,
            phase->m_renderGroups, phase->m_commandBuffer})
        .build();
}

RenderTargets DefaultRenderPhaseProvider::createRenderTargets(core::device::DeviceContext &context,
                                                              RenderingContext &ctx)
{
    return RenderTargets::forOffscreen(context, ctx);
}
} // namespace star::core::renderer