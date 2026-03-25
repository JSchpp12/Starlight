#include "starlight/core/renderer/HeadlessRenderer.hpp"

#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/command/command_order/TriggerPass.hpp"

namespace star::core::renderer
{
namespace pre_pass
{
vk::ImageMemoryBarrier2 GetImageFromNeighbor::getBarrier() const noexcept
{
    assert(targetTexture != nullptr && "The target texture must be provided during init");
    return vk::ImageMemoryBarrier2();
}
} // namespace pre_pass

namespace post_pass
{
vk::ImageMemoryBarrier2 PrepImageForNeighbor::getBarrier() const noexcept
{
    assert(targetTexture != nullptr && "The target texture must be provided during init");
    return vk::ImageMemoryBarrier2();
}
} // namespace post_pass

void HeadlessRenderer::frameUpdate(common::IDeviceContext &c)
{
    this->DefaultRenderer::frameUpdate(c);

    auto &context = static_cast<star::core::device::DeviceContext &>(c);

    auto semaphore =
        context.getManagerCommandBuffer()
            .m_manager.get(m_commandBuffer)
            .commandBuffer->getCompleteSemaphores()[context.getFrameTracker().getCurrent().getFrameInFlightIndex()];

    context.getCmdBus().submit(
        star::command_order::TriggerPass().setBinarySemaphore(semaphore).setPass(m_commandBuffer));
}

core::device::manager::ManagerCommandBuffer::Request HeadlessRenderer::getCommandBufferRequest()
{
    return core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&HeadlessRenderer::recordCommandBuffer, this, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3),
        .order = Command_Buffer_Order::main_render_pass,
        .orderIndex = Command_Buffer_Order_Index::first,
        .type = Queue_Type::Tgraphics,
        .waitStage = m_waitPoint,
        .willBeSubmittedEachFrame = true,
        .recordOnce = false};
}

static void PostApplyPipelineBarriers(vk::CommandBuffer &commandBuffer,
                                      const star::StarTextures::Texture &renderToColorTexture,
                                      uint32_t neighborQueueFamilyIndex)
{
}

static void PreApplyPipelineBarriers(vk::CommandBuffer &commandBuffer,
                                     const star::StarTextures::Texture &renderToColorTexture,
                                     uint32_t neighborQueueFamilyIndex)
{

    const vk::ImageMemoryBarrier2 barriers[1]{vk::ImageMemoryBarrier2()
                                                  .setImage(renderToColorTexture.getVulkanImage())
                                                  .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                                                  .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                                                  .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
                                                  .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)};
}

void HeadlessRenderer::prepRender(common::IDeviceContext &c)
{
    this->DefaultRenderer::prepRender(c);

    const auto &context = static_cast<star::core::device::DeviceContext &>(c);
    m_cachedBus = &context.getCmdBus();

    m_prepScheme.resize(context.getFrameTracker().getSetup().getNumFramesInFlight(), pre_pass::DoNothing{});
    m_postScheme.resize(context.getFrameTracker().getSetup().getNumFramesInFlight(), post_pass::DoNothing{});
}

void HeadlessRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                      const uint64_t &frameIndex)
{
    //// get neighbor information
    // Handle neighbor;
    const size_t ii = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    // assert(m_cachedBus != nullptr);
    //{
    //     auto cmd = star::command_order::GetPassInfo{m_commandBuffer};
    //     m_cachedBus->submit(cmd);

    //    const auto &ele = cmd.getReply().get();
    //    if (ele.edges != nullptr)
    //    {
    //        // only checking for one neighbor right now
    //        auto &consumer = ele.edges->front();

    //        for (const auto &edge : *cmd.getReply().get().edges)
    //        {
    //            if (edge.producer == m_commandBuffer)
    //            {
    //                // need to check if neighbor was run this frame
    //                auto nCmd = star::command_order::GetPassInfo{edge.consumer};
    //                if (nCmd.getReply().get().isTriggeredThisFrame)
    //                {
    //                    // next frame will have to acquire
    //                }
    //                // this is producing something for some other buffer
    //                // right now assuming that it is the color texture
    //                m_prepScheme = pre_pass::GetImageFromNeighbor()
    //            }
    //        }
    //    }

    //    m_prepScheme = pre_pass::DoNothing{};
    //}

    const auto *renderColor = m_renderingContext.recordDependentImage.get(
        m_renderToImages[frameTracker.getCurrent().getFrameInFlightIndex()]);
    assert(renderColor && "Color texture was never prepared in the rendering context");

    if (std::holds_alternative<pre_pass::GetImageFromNeighbor>(m_prepScheme[ii]))
    {
    }
    // check neighbors and see if they are on a different queue
    //
    // PreApplyPipelineBarriers(commandBuffer, *renderColor);

    this->DefaultRenderer::recordCommands(commandBuffer, frameTracker, frameIndex);

    if (std::holds_alternative<post_pass::PrepImageForNeighbor>(m_postScheme[ii]))
    {
    }

    // PostApplyPipelineBarriers(commandBuffer, *renderColor);

    const size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    m_renderingContext.recordDependentImage.get(m_renderToImages[index])
        ->setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
}
} // namespace star::core::renderer