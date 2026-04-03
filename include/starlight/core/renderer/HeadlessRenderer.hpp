#pragma once

#include <starlight/core/renderer/DefaultRenderer.hpp>

#include <variant>

namespace star::core::renderer
{
namespace pre_pass
{
struct DoNothing
{
};
struct GetImageFromNeighbor
{
    uint32_t myQueueFamilyIndex;
    uint32_t neighborQueueFamilyIndex;
    StarTextures::Texture *targetTexture{nullptr};

    vk::ImageMemoryBarrier2 getBarrier() const noexcept;
};
} // namespace pre_pass

namespace post_pass
{
struct DoNothing
{
};
struct PrepImageForNeighbor
{
    uint32_t myQueueFamilyIndex;
    uint32_t neighborQueueFamilyIndex;
    StarTextures::Texture *targetTexture{nullptr};

    vk::ImageMemoryBarrier2 getBarrier() const noexcept;
};
} // namespace post_pass

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
                     std::vector<std::shared_ptr<StarObject>> objects, std::shared_ptr<std::vector<Light>> lights,
                     std::shared_ptr<StarCamera> camera, vk::PipelineStageFlags waitPoint)
        : star::core::renderer::DefaultRenderer(context, numFramesInFlight, lights, camera, objects),
          m_waitPoint(waitPoint)
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
    HeadlessRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                     std::vector<std::shared_ptr<StarObject>> objects,
                     std::shared_ptr<ManagerController::RenderResource::Buffer> lightData,
                     std::shared_ptr<ManagerController::RenderResource::Buffer> lightListData,
                     std::shared_ptr<ManagerController::RenderResource::Buffer> cameraData,
                     vk::PipelineStageFlags waitPoint)
        : star::core::renderer::DefaultRenderer(context, numFramesInFlight, std::move(objects), std::move(lightData),
                                                std::move(lightListData), std::move(cameraData)),
          m_waitPoint(waitPoint)
    {
    }
    HeadlessRenderer(const HeadlessRenderer &) = delete;
    HeadlessRenderer &operator=(const HeadlessRenderer &) = delete;
    HeadlessRenderer(HeadlessRenderer &&) = default;
    HeadlessRenderer &operator=(HeadlessRenderer &&) = default;
    virtual ~HeadlessRenderer() = default;

    virtual void frameUpdate(common::IDeviceContext &c) override;

    virtual void prepRender(common::IDeviceContext &c) override;

    const std::vector<Handle> &getTimelineSemaphores() const
    {
        return m_timelineSemaphores;
    }

  private:
    vk::PipelineStageFlags m_waitPoint{vk::PipelineStageFlagBits::eFragmentShader};
    std::vector<std::variant<pre_pass::DoNothing, pre_pass::GetImageFromNeighbor>> m_prepScheme;
    std::vector<std::variant<post_pass::DoNothing, post_pass::PrepImageForNeighbor>> m_postScheme;
    std::vector<Handle> m_timelineSemaphores;
    vk::Device m_device{VK_NULL_HANDLE};
    const star::core::device::manager::Image *m_imgMgr{nullptr};
    const star::core::CommandBus *m_cmdBus{nullptr};

    //void applyPrePipelineBarriers(vk::CommandBuffer commandBuffer, const star::common::FrameTracker &ft) const;

    void waitForSemaphore(const common::FrameTracker &Ft) const;

    core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

    virtual void recordCommandBuffer(star::StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                const uint64_t &frameIndex) override;

    vk::Semaphore submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue);
};
} // namespace star::core::renderer