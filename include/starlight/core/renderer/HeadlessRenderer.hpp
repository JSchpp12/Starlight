#pragma once

#include <starlight/core/renderer/DefaultRenderer.hpp>

namespace star::core::renderer
{
class HeadlessRenderer : public star::core::renderer::DefaultRenderer
{
  public:
    HeadlessRenderer() = default;
    HeadlessRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::vector<std::shared_ptr<StarObject>> objects, std::shared_ptr<std::vector<Light>> lights,
                     std::shared_ptr<StarCamera> camera)
        : star::core::renderer::DefaultRenderer(context, numFramesInFlight, lights, camera, objects)
    {
    }
    HeadlessRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::vector<std::shared_ptr<StarObject>> objects,
                     std::shared_ptr<ManagerController::RenderResource::Buffer> lightData,
                     std::shared_ptr<ManagerController::RenderResource::Buffer> lightListData,
                     std::shared_ptr<ManagerController::RenderResource::Buffer> cameraData)
        : star::core::renderer::DefaultRenderer(context, numFramesInFlight, std::move(objects), std::move(lightData),
                                                std::move(lightListData), std::move(cameraData))
    {
    }
    HeadlessRenderer(const HeadlessRenderer &) = delete;
    HeadlessRenderer &operator=(const HeadlessRenderer &) = delete;
    HeadlessRenderer(HeadlessRenderer &&) = default;
    HeadlessRenderer &operator=(HeadlessRenderer &&) = default;
    virtual ~HeadlessRenderer() = default;

  private:
    core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

    void recordCommands(vk::CommandBuffer &commandBuffer, const common::FrameTracker &frameTracker,
                             const uint64_t &frameIndex) override;

    //vk::Semaphore forceSkipSubmission(StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
    //                         std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
    //                         std::vector<vk::Semaphore> dataSemaphores,
    //                         std::vector<vk::PipelineStageFlags> dataWaitPoints,
    //                         std::vector<std::optional<uint64_t>> previousSignaledValues);
};
} // namespace star::core::renderer