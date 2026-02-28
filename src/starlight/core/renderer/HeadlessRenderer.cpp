#include "starlight/core/renderer/HeadlessRenderer.hpp"

namespace star::core::renderer
{
core::device::manager::ManagerCommandBuffer::Request HeadlessRenderer::getCommandBufferRequest()
{
    return core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&HeadlessRenderer::recordCommandBuffer, this, std::placeholders::_1,
                                          std::placeholders::_2, std::placeholders::_3),
        .order = Command_Buffer_Order::main_render_pass,
        .orderIndex = Command_Buffer_Order_Index::first,
        .type = Queue_Type::Tgraphics,
        .waitStage = vk::PipelineStageFlagBits::eFragmentShader,
        .willBeSubmittedEachFrame = true,
        .recordOnce = false};
}

void HeadlessRenderer::recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                                           const uint64_t &frameIndex)
{
    this->DefaultRenderer::recordCommands(commandBuffer, frameTracker, frameIndex);

    const size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    m_renderingContext.recordDependentImage.get(m_renderToImages[index])
        ->setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
}

// vk::Semaphore HeadlessRenderer::forceSkipSubmission(StarCommandBuffer &buffer, const common::FrameTracker
// &frameTracker,
//                                                     std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
//                                                     std::vector<vk::Semaphore> dataSemaphores,
//                                                     std::vector<vk::PipelineStageFlags> dataWaitPoints,
//                                                     std::vector<std::optional<uint64_t>> previousSignaledValues)
//{
//     const size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
//     return previousCommandBufferSemaphores->front();
// }
} // namespace star::core::renderer