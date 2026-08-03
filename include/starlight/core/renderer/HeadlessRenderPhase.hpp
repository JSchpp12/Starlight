#pragma once

#include <starlight/core/renderer/DefaultRenderPhase.hpp>

#include <variant>

namespace star::core::renderer
{
class HeadlessRenderPhaseProvider;

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

class HeadlessRenderPhase : public DefaultRenderPhase
{
  public:
    HeadlessRenderPhase() = default;
    virtual ~HeadlessRenderPhase() = default;

    HeadlessRenderPhase(const HeadlessRenderPhase &) = delete;
    HeadlessRenderPhase &operator=(const HeadlessRenderPhase &) = delete;
    HeadlessRenderPhase(HeadlessRenderPhase &&) = delete;
    HeadlessRenderPhase &operator=(HeadlessRenderPhase &&) = delete;

    virtual void frameUpdate(common::IDeviceContext &c) override;

    virtual void recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                     const uint64_t &frameIndex) override;

    const Handle &getSemaphores(size_t index) const
    {
        assert(index < m_timelineSemaphores.size());
        return m_timelineSemaphores[index];
    }

  protected:
    virtual std::optional<core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> getSubmissionOverride()
        override;

  private:
    friend class HeadlessRenderPhaseProvider;

    std::vector<std::variant<pre_pass::DoNothing, pre_pass::GetImageFromNeighbor>> m_prepScheme;
    std::vector<std::variant<post_pass::DoNothing, post_pass::PrepImageForNeighbor>> m_postScheme;
    std::vector<Handle> m_timelineSemaphores;
    vk::Device m_device{VK_NULL_HANDLE};
    const star::core::device::manager::Image *m_imgMgr{nullptr};
    const star::core::CommandBus *m_cmdBus{nullptr};

    void waitForSemaphore(const common::FrameTracker &ft) const;

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues, StarQueue &queue);
};
} // namespace star::core::renderer
