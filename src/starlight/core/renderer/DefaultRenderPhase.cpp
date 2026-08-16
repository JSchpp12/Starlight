#include "renderer/DefaultRenderPhase.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "ManagerRenderResource.hpp"
#include "core/device/system/event/ManagerRequest.hpp"
#include "core/helper/command_buffer/CommandBufferHelpers.hpp"
#include "core/helper/queue/QueueHelpers.hpp"
#include "starlight/core/waiter/one_shot/GenericEvent.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <vma/vk_mem_alloc.h>

#include <algorithm>
#include <optional>
#include <vector>

namespace star::core::renderer
{
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

void DefaultRenderPhase::updateDependentData(star::core::device::DeviceContext &context)
{
    if (m_barrFunction == nullptr)
        return;

    auto result = m_frameData->frameUpdate(context);
    auto &record = context.getManagerCommandBuffer().m_manager.get(m_commandBuffer);
    for (const auto &w : result.waits)
    {
        record.oneTimeWaitSemaphoreInfo.insert(w.handle, w.semaphore, w.waitStage, w.signalValue);
        m_renderingContext.addBufferToRenderingContext(context, w.handle);
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

    recordCommandBufferDependencies(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

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
                                                         const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
    if (m_barrFunction == nullptr)
        return;

    size_t barrCount{0};
    m_barrFunction(frameInFlightIndex, frameIndex, m_frameData.get(), &m_dataRoles, &m_renderingContext,
                   m_runtimeBarriers.data(), &barrCount);

    commandBuffer.pipelineBarrier2(
        vk::DependencyInfo().setBufferMemoryBarrierCount(barrCount).setPBufferMemoryBarriers(m_runtimeBarriers.data()));
}

void DefaultRenderPhase::AddOwnsAllResourcesBarrier(uint8_t frameInFlightIndex, const uint64_t &frameIndex,
                                                    const FrameData *fd, const DataRoles *roles,
                                                    const RenderingContext *rc, vk::BufferMemoryBarrier2 *data,
                                                    size_t *dCount) noexcept
{
    const auto *camera = fd->controller(roles->camera);
    const auto *lightInfo = fd->controller(roles->lightInfo);
    const auto *lightList = fd->controller(roles->lightList);

    if (camera->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
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

    if (lightInfo->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
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

    if (lightList->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
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

DefaultRenderPhase &DefaultRenderPhase::setDataRolesOwned(Handle cameraRole, Handle lightInfoRole, Handle lightListRole)
{
    m_dataRoles = DataRoles{.camera = cameraRole, .lightInfo = lightInfoRole, .lightList = lightListRole};

    assert(m_frameData && "Frame data needs to be assigned first");
    assert(m_frameData->isResourceDriven(m_dataRoles.camera) && "owned camera role must be a driven buffer");
    assert(m_frameData->isResourceDriven(m_dataRoles.lightInfo) && "owned lightInfo role must be a driven buffer");
    assert(m_frameData->isResourceDriven(m_dataRoles.lightList) && "owned lightList role must be a driven buffer");
    m_barrFunction = &AddOwnsAllResourcesBarrier;

    return *this;
}

DefaultRenderPhase &DefaultRenderPhase::setDataRolesBorrowed(Handle cameraRole, Handle lightInfoRole,
                                                             Handle lightListRole)
{
    m_dataRoles = DataRoles{.camera = cameraRole, .lightInfo = lightInfoRole, .lightList = lightListRole};
    m_barrFunction = nullptr;

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