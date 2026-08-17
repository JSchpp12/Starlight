#include "renderer/DefaultRenderPhase.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "ManagerRenderResource.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/helper/command_buffer/CommandBufferHelpers.hpp"
#include "core/helper/queue/QueueHelpers.hpp"
#include "core/renderer/DescriptorRecipe.hpp"
#include "starlight/command/command_order/DeclarePass.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/renderer/FrameData.hpp"
#include "starlight/core/waiter/one_shot/CreateDescriptorsOnEventPolicy.hpp"
#include "starlight/core/waiter/one_shot/GenericEvent.hpp"
#include "starlight/event/DescriptorPoolReady.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <vma/vk_mem_alloc.h>

#include <algorithm>
#include <cassert>
#include <functional>
#include <optional>
#include <vector>

namespace star::core::renderer
{
static void RegisterWithCommandOrder(const star::core::CommandBus &cmdBus, star::common::EventBus &evtBus,
                                     star::core::device::manager::Queue &qm, Handle commandBuffer)
{
    auto *queue = star::core::helper::GetEngineDefaultQueue(evtBus, qm, star::Queue_Type::Tgraphics);
    assert(queue != nullptr && "Failed to acquire default engine queue");

    cmdBus.submit(star::command_order::DeclarePass{std::move(commandBuffer), queue->getParentQueueFamilyIndex()});
}

static std::vector<star::StarRenderGroup> CreateRenderingGroups(core::device::DeviceContext &context,
                                                                std::vector<std::shared_ptr<star::StarObject>> objects)
{
    auto renderingGroups = std::vector<star::StarRenderGroup>();

    for (size_t i = 0; i < objects.size(); i++)
    {
        star::StarRenderGroup *match = nullptr;

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

static const star::core::device::manager::ImageRecord *GetImg(const star::Handle &handle,
                                                              core::device::DeviceContext &context)
{
    const auto *vRec = context.getImageManager().get(handle);
    if (vRec == nullptr)
        STAR_THROW("Failed to retreive color texture from manager. The factory method should have registered all "
                   "textures with the manager");

    return vRec;
}

static RenderTargets createRenderTargets(core::device::DeviceContext &context, RenderingContext &ctx)
{
    auto targets = RenderTargets::forOffscreen(context, ctx);

    auto *graphicsQueueToUse = core::helper::GetEngineDefaultQueue(
        context.getEventBus(), context.getGraphicsManagers().queueManager, star::Queue_Type::Tpresent);
    assert(graphicsQueueToUse != nullptr);

    std::vector<vk::ImageMemoryBarrier2> imgBarriers(targets.colorHandles().size() + targets.depthHandles().size());
    size_t imgIndex = 0;
    for (size_t i = 0; i < targets.colorHandles().size(); i++)
    {
        const auto *vRec = GetImg(targets.colorHandles()[i], context);
        imgBarriers[imgIndex++] = vk::ImageMemoryBarrier2()
                                      .setOldLayout(vk::ImageLayout::eUndefined)
                                      .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
                                      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                      .setImage(vRec->texture.getVulkanImage())
                                      .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                                      .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                                      .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite |
                                                        vk::AccessFlagBits2::eColorAttachmentRead)
                                      .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
                                      .setSubresourceRange(vk::ImageSubresourceRange()
                                                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                               .setBaseMipLevel(0)
                                                               .setLevelCount(vk::RemainingMipLevels)
                                                               .setBaseArrayLayer(0)
                                                               .setLayerCount(vk::RemainingArrayLayers));
    }

    for (size_t i = 0; i < targets.depthHandles().size(); i++)
    {
        const auto *vRec = GetImg(targets.depthHandles()[i], context);
        imgBarriers[imgIndex++] = vk::ImageMemoryBarrier2()
                                      .setOldLayout(vk::ImageLayout::eUndefined)
                                      .setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                      .setImage(vRec->texture.getVulkanImage())
                                      .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                                      .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                                      .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                                      .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
                                      .setSubresourceRange(vk::ImageSubresourceRange()
                                                               .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                               .setBaseMipLevel(0)
                                                               .setLevelCount(vk::RemainingMipLevels)
                                                               .setBaseArrayLayer(0)
                                                               .setLayerCount(vk::RemainingArrayLayers));
    }

    auto oneTimeSetup = star::core::helper::BeginSingleTimeCommands(context.getDevice(), context.getEventBus(),
                                                                    context.getManagerCommandBuffer().m_manager,
                                                                    star::Queue_Type::Tgraphics);

    oneTimeSetup.buffer().pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(imgBarriers));

    core::helper::EndSingleTimeCommands(*graphicsQueueToUse, std::move(oneTimeSetup));
    return targets;
}

DefaultRenderPhase::Builder::Builder(core::device::DeviceContext &context) : m_context(context)
{
}

DefaultRenderPhase::Builder &DefaultRenderPhase::Builder::setObjects(std::vector<std::shared_ptr<StarObject>> objects)
{
    m_objects = std::move(objects);
    return *this;
}

DefaultRenderPhase::Builder &DefaultRenderPhase::Builder::setFrameData(std::shared_ptr<FrameData> frameData)
{
    m_frameData = std::move(frameData);
    return *this;
}

DefaultRenderPhase::Builder &DefaultRenderPhase::Builder::setOwnsFrameData(bool owned)
{
    m_ownsFrameData = owned;
    return *this;
}

DefaultRenderPhase::Builder &DefaultRenderPhase::Builder::setConfig(RenderPhaseConfig config)
{
    m_config = config;
    return *this;
}

DefaultRenderPhase::Builder &DefaultRenderPhase::Builder::setRenderTargetsFactory(
    std::function<RenderTargets(RenderingContext &)> factory)
{
    m_renderTargetsFactory = std::move(factory);
    return *this;
}

std::unique_ptr<DefaultRenderPhase> DefaultRenderPhase::Builder::buildUnique()
{
    auto phase = std::make_unique<DefaultRenderPhase>();
    buildInto(*phase);
    return phase;
}

void DefaultRenderPhase::Builder::buildInto(DefaultRenderPhase &target)
{
    target.m_objects = std::move(m_objects);
    target.m_frameData = m_frameData;
    target.setDataRoles(m_ownsFrameData);

    target.m_renderGroups = CreateRenderingGroups(m_context, target.m_objects);

    auto request = core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&DefaultRenderPhase::recordCommandBuffer, &target, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3),
        .order = m_config.order,
        .orderIndex = m_config.orderIndex,
        .type = m_config.queueType,
        .waitStage = m_config.waitStage,
        .willBeSubmittedEachFrame = m_config.willBeSubmittedEachFrame,
        .recordOnce = m_config.recordOnce,
        .overrideBufferSubmissionCallback = target.getSubmissionOverride(),
    };
    target.m_commandBuffer = m_context.getManagerCommandBuffer().submit(
        std::move(request), m_context.frameTracker().getCurrent().getGlobalFrameCounter());
    RegisterWithCommandOrder(m_context.getCmdBus(), m_context.getEventBus(),
                             m_context.getGraphicsManagers().queueManager, target.m_commandBuffer);

    target.m_frameData->prepRender(m_context, m_context.frameTracker().getSetup().getNumFramesInFlight());

    target.m_renderingContext.targetResolution = m_context.getEngineResolution();
    if (m_renderTargetsFactory)
        target.m_renderTargets = m_renderTargetsFactory(target.m_renderingContext);
    else
        target.m_renderTargets = createRenderTargets(m_context, target.m_renderingContext);

    for (auto &group : target.m_renderGroups)
        group.prepRender(m_context);

    const auto global = shaderInfoHandle("Global");
    DescriptorRecipe::Builder(m_context.getEventBus(), m_context, star::event::DescriptorPoolReady::GetUniqueTypeName())
        .setShaderInfoOut(global, &target.m_globalShaderInfo)
        .addBinding(global, 0, target.m_frameData, roleHandle(frame_roles::Camera), 0,
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(global, 0, target.m_frameData, roleHandle(frame_roles::LightInfo), 1,
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(global, 0, target.m_frameData, roleHandle(frame_roles::LightList), 2,
                    vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
        .setRenderGroups(global, &target.m_renderGroups, target.getRenderTargetInfo(), target.m_commandBuffer)
        .build();
}

void DefaultRenderPhase::frameUpdate(common::IDeviceContext &context)
{
    auto &c = static_cast<core::device::DeviceContext &>(context);
    size_t i = static_cast<size_t>(c.frameTracker().getCurrent().getFrameInFlightIndex());
    m_renderTargets.frameUpdate(c, m_renderingContext);

    updateDependentData(c);
    RenderPhase::frameUpdate(context);
}

void DefaultRenderPhase::cleanupRender(common::IDeviceContext &context)
{
    // Clean the render groups first: this destroys every pipeline layout, which references the global set layout. Only
    // after the pipeline layouts are gone is it safe to release the global set layout owned by m_globalShaderInfo.
    RenderPhase::cleanupRender(context);

    auto &c = static_cast<core::device::DeviceContext &>(context);
    if (m_globalShaderInfo)
    {
        m_globalShaderInfo->cleanupRender(c.getDevice());
        m_globalShaderInfo.reset();
    }
}

void DefaultRenderPhase::recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                             const uint64_t &frameIndex)
{
    commandBuffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());

    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);

    commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()).end();
}

void DefaultRenderPhase::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                              const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        // Bind the global descriptor set (camera/lights) once for this render group's pipeline layout before any object
        // records its per-mesh draws. Previously every mesh rebound the global set (and the per-object instance set)
        // via the material; now only the material set is rebound per mesh.
        if (m_globalShaderInfo)
        {
            auto globalSets = m_globalShaderInfo->getDescriptors(frameInFlightIndex);
            if (!globalSets.empty())
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, group.getPipelineLayout(), 0,
                                                 globalSets.size(), globalSets.data(), 0, nullptr);
            }
        }

        group.recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void DefaultRenderPhase::recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                        const uint64_t &frameIndex)
{
    vk::Viewport viewport = this->prepareRenderingViewport(m_renderingContext.targetResolution);
    commandBuffer.setViewport(0, viewport);

    recordPreRenderPassCommands(commandBuffer, frameTracker);

    recordCommandBufferDependencies(commandBuffer, frameTracker, frameIndex);

    {
        vk::RenderingAttachmentInfo colorAttachments;
        std::optional<vk::RenderingAttachmentInfo> depthAttachment;
        if (m_renderTargets.hasColor())
            colorAttachments = prepareDynamicRenderingInfoColorAttachment(frameTracker);
        if (m_renderTargets.hasDepth())
            depthAttachment = prepareDynamicRenderingInfoDepthAttachment(frameTracker);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, m_renderingContext.targetResolution};
        vk::RenderingInfoKHR renderInfo{};
        renderInfo.renderArea = renderArea;
        renderInfo.layerCount = 1;
        renderInfo.pDepthAttachment = depthAttachment ? &*depthAttachment : nullptr;
        renderInfo.pColorAttachments = &colorAttachments;
        renderInfo.colorAttachmentCount = m_renderTargets.hasColor() ? 1 : 0;
        commandBuffer.beginRendering(renderInfo);
    }

    recordRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    commandBuffer.endRendering();

    recordPostRenderingCalls(commandBuffer, frameTracker);
}

void DefaultRenderPhase::recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer,
                                                         const common::FrameTracker &frameTracker,
                                                         const uint64_t &frameIndex)
{
    if (m_barrFunction == nullptr)
        return;

    size_t barrCount{0};
    m_barrFunction(frameTracker, frameIndex, m_frameData.get(), &m_renderingContext, m_runtimeBarriers.data(),
                   &barrCount);

    commandBuffer.pipelineBarrier2(
        vk::DependencyInfo().setBufferMemoryBarrierCount(barrCount).setPBufferMemoryBarriers(m_runtimeBarriers.data()));
}

void DefaultRenderPhase::AddOwnsAllResourcesBarrier(const common::FrameTracker &frameTracker,
                                                    const uint64_t &frameIndex, const FrameData *fd,
                                                    const RenderingContext *rc, vk::BufferMemoryBarrier2 *data,
                                                    size_t *dCount) noexcept
{
    const uint8_t frameInFlightIndex = frameTracker.getCurrent().getFrameInFlightIndex();

    const auto *camera = fd->getController(roleHandle(frame_roles::Camera));
    const auto *lightInfo = fd->getController(roleHandle(frame_roles::LightInfo));
    const auto *lightList = fd->getController(roleHandle(frame_roles::LightList));

    if (camera->willBeUpdatedThisFrame(frameIndex, frameTracker))
    {
        auto buffer = rc->bufferTransferRecords.get(camera->getHandle(frameInFlightIndex));

        *(data++) = vk::BufferMemoryBarrier2()
                        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                         vk::PipelineStageFlagBits2::eVertexShader)
                        .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setBuffer(buffer)
                        .setSize(vk::WholeSize);
        (*dCount)++;
    }

    if (lightInfo->willBeUpdatedThisFrame(frameIndex, frameTracker))
    {
        *(data++) = vk::BufferMemoryBarrier2()
                        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                         vk::PipelineStageFlagBits2::eVertexShader)
                        .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setBuffer(rc->bufferTransferRecords.get(lightInfo->getHandle(frameInFlightIndex)))
                        .setSize(vk::WholeSize);
        (*dCount)++;
    }

    if (lightList->willBeUpdatedThisFrame(frameIndex, frameTracker))
    {
        *(data++) = vk::BufferMemoryBarrier2()
                        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                         vk::PipelineStageFlagBits2::eVertexShader)
                        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setBuffer(rc->bufferTransferRecords.get(lightList->getHandle(frameInFlightIndex)))
                        .setSize(vk::WholeSize);
        (*dCount)++;
    }
}

DefaultRenderPhase &DefaultRenderPhase::setDataRoles(bool owned)
{
    if (owned)
    {
        assert(m_frameData && "Frame data needs to be assigned first");
        assert(m_frameData->isResourceDriven(roleHandle(frame_roles::Camera)) &&
               "owned camera role must be a driven buffer");
        assert(m_frameData->isResourceDriven(roleHandle(frame_roles::LightInfo)) &&
               "owned lightInfo role must be a driven buffer");
        assert(m_frameData->isResourceDriven(roleHandle(frame_roles::LightList)) &&
               "owned lightList role must be a driven buffer");

        m_drivesFrameData = true;
        m_barrFunction = &AddOwnsAllResourcesBarrier;
    }
    else
    {
        m_drivesFrameData = false;
        m_barrFunction = nullptr;
    }

    return *this;
}

vk::RenderingAttachmentInfo star::core::renderer::DefaultRenderPhase::prepareDynamicRenderingInfoColorAttachment(
    const common::FrameTracker &frameTracker)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    const auto *r = m_renderingContext.recordDependentImage.get(m_renderTargets.colorHandles()[index]);

    vk::RenderingAttachmentInfoKHR colorAttachmentInfo{};
    colorAttachmentInfo.imageView = r->getImageView();
    colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfo.clearValue = vk::ClearValue{vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

    return colorAttachmentInfo;
}

vk::RenderingAttachmentInfo star::core::renderer::DefaultRenderPhase::prepareDynamicRenderingInfoDepthAttachment(
    const common::FrameTracker &frameTracker)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo{};
    depthAttachmentInfo.imageView =
        m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[index])->getImageView();
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.clearValue = vk::ClearValue{vk::ClearDepthStencilValue{1.0f}};

    return depthAttachmentInfo;
}

vk::Viewport DefaultRenderPhase::prepareRenderingViewport(const vk::Extent2D &resolution)
{
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)resolution.width;
    viewport.height = (float)resolution.height;
    // Specify values range of depth values to use for the framebuffer. If not doing anything special, leave at
    // default
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

} // namespace star::core::renderer