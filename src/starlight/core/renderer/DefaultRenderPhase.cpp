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
namespace star::core::renderer
{
star::StarShaderInfo::Builder DefaultRenderPhase::manualCreateDescriptors(star::core::device::DeviceContext &context)
{
    assert(m_infoManagerCamera &&
           "Camera info does not always need to exist. But it should. Hitting this means a change is needed");

    StarDescriptorPool *defaultPool{nullptr};
    {
        const Handle dHandle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                 core::device::manager::GetDescriptorPoolTypeName),
                             .id = 0};

        defaultPool = context.getDescriptorPoolManager().get(dHandle)->pool.get();
    }

    assert(defaultPool != nullptr &&
           "Pool has not been created yet. Descriptor pools are created after engine prep phase is complete");

    const uint8_t numFramesInFlight = context.frameTracker().getSetup().getNumFramesInFlight();
    this->globalSetLayout = createGlobalDescriptorSetLayout(context, numFramesInFlight);

    // Build the global StarShaderInfo once and own it on the phase. It is bound a
    // single time per frame in recordRenderingCalls instead of being duplicated
    // into every material and rebound for every mesh.
    {
        auto globalBuilder =
            StarShaderInfo::Builder(context.getDeviceID(), context.getDevice(), *defaultPool, numFramesInFlight)
                .addSetLayout(this->globalSetLayout);
        for (int i = 0; i < numFramesInFlight; i++)
        {
            const auto &lightInfoHandle = m_infoManagerLightData->getHandle(i);
            const auto &lightListHandle = m_infoManagerLightList->getHandle(i);
            const auto &cameraHandle = m_infoManagerCamera->getHandle(i);

            globalBuilder.startOnFrameIndex(i)
                .startSet()
                .add(star::StarShaderInfo::BufferInfo{cameraHandle})
                .add(star::StarShaderInfo::BufferInfo{lightInfoHandle})
                .add(star::StarShaderInfo::BufferInfo{lightListHandle});
        }
        m_globalShaderInfo = globalBuilder.build();
    }

    // Return a builder carrying only the global set layout. Render groups use it to
    // assemble the pipeline layout; per-object and per-mesh sets are now built by
    // the objects and materials themselves.
    return StarShaderInfo::Builder(context.getDeviceID(), context.getDevice(), *defaultPool, numFramesInFlight)
        .addSetLayout(this->globalSetLayout);
}

std::shared_ptr<star::StarDescriptorSetLayout> DefaultRenderPhase::createGlobalDescriptorSetLayout(
    device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    return StarDescriptorSetLayout::Builder()
        .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
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
    // Clean the render groups first: this destroys every pipeline layout, which
    // references the global set layout. Only after the pipeline layouts are gone
    // is it safe to release the global set layout owned by m_globalShaderInfo.
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
    if (!ownsRenderResourceControllers)
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
        // Bind the global descriptor set (camera/lights) once for this render
        // group's pipeline layout before any object records its per-mesh draws.
        // Previously every mesh rebound the global set (and the per-object
        // instance set) via the material; now only the material set is rebound
        // per mesh.
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
        vk::RenderingAttachmentInfo colorAttachmentInfo = prepareDynamicRenderingInfoColorAttachment(frameTracker);
        vk::RenderingAttachmentInfo depthAttachmentInfo = prepareDynamicRenderingInfoDepthAttachment(frameTracker);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, m_renderingContext.targetResolution};
        vk::RenderingInfoKHR renderInfo{};
        renderInfo.renderArea = renderArea;
        renderInfo.layerCount = 1;
        renderInfo.pDepthAttachment = &depthAttachmentInfo;
        renderInfo.pColorAttachments = &colorAttachmentInfo;
        renderInfo.colorAttachmentCount = 1;
        commandBuffer.beginRendering(renderInfo);
    }

    recordRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    commandBuffer.endRendering();

    recordPostRenderingCalls(commandBuffer, frameTracker);
}

void DefaultRenderPhase::recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer,
                                                         const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
    auto memoryBarriers = getMemoryBarriersForThisFrame(frameInFlightIndex, frameIndex);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                       .setBufferMemoryBarrierCount(memoryBarriers.size())
                                       .setPBufferMemoryBarriers(memoryBarriers.data()));
}

std::vector<vk::BufferMemoryBarrier2> DefaultRenderPhase::getMemoryBarriersForThisFrame(
    const uint8_t &frameInFlightIndex, const uint64_t &frameIndex)
{
    auto barriers = std::vector<vk::BufferMemoryBarrier2>();

    if (ownsRenderResourceControllers)
    {
        if (m_infoManagerCamera->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
        {
            auto buffer =
                m_renderingContext.bufferTransferRecords.get(m_infoManagerCamera->getHandle(frameInFlightIndex));

            barriers.emplace_back(
                vk::BufferMemoryBarrier2()
                    .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                    .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                    .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                     vk::PipelineStageFlagBits2::eVertexShader)
                    .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                    .setBuffer(buffer)
                    .setSize(vk::WholeSize));
        }

        if (m_infoManagerLightData->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
        {
            barriers.emplace_back(
                vk::BufferMemoryBarrier2()
                    .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                    .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                    .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                     vk::PipelineStageFlagBits2::eVertexShader)
                    .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                    .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                    .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                    .setBuffer(m_renderingContext.bufferTransferRecords.get(
                        m_infoManagerLightData->getHandle(frameInFlightIndex)))
                    .setSize(vk::WholeSize));
        }

        if (m_infoManagerLightList->willBeUpdatedThisFrame(frameIndex, frameInFlightIndex))
        {
            barriers.emplace_back(vk::BufferMemoryBarrier2()
                                      .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                                      .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                                      .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                                       vk::PipelineStageFlagBits2::eVertexShader)
                                      .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                      .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                      .setBuffer(m_renderingContext.bufferTransferRecords.get(
                                          m_infoManagerLightList->getHandle(frameInFlightIndex)))
                                      .setSize(vk::WholeSize));
        }
    }

    return barriers;
}

vk::RenderingAttachmentInfo star::core::renderer::DefaultRenderPhase::prepareDynamicRenderingInfoColorAttachment(
    const common::FrameTracker &frameTracker)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    const auto *r = m_renderingContext.recordDependentImage.get(m_renderToImages[index]);

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
        m_renderingContext.recordDependentImage.get(m_renderToDepthImages[index])->getImageView();
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
