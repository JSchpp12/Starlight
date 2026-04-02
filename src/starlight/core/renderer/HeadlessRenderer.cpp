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

void HeadlessRenderer::frameUpdate(common::IDeviceContext &c)
{
    this->DefaultRenderer::frameUpdate(c);

    auto &context = static_cast<star::core::device::DeviceContext &>(c);

    auto semaphore =
        context.getManagerCommandBuffer()
            .m_manager.get(m_commandBuffer)
            .commandBuffer->getCompleteSemaphores()[context.getFrameTracker().getCurrent().getFrameInFlightIndex()];

    size_t ii = static_cast<size_t>(context.getFrameTracker().getCurrent().getFrameInFlightIndex());

    context.getCmdBus().submit(
        star::command_order::TriggerPass()
            .setTimelineSemaphore(m_timelineSemaphores[ii])
            .setSignalValue(context.getFrameTracker().getCurrent().getNumTimesFrameProcessed() + 1)
            .setPass(m_commandBuffer));
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
        .recordOnce = false,
        .overrideBufferSubmissionCallback = std::bind(
            &HeadlessRenderer::submitBuffer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
            std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7)};
}

void HeadlessRenderer::waitForSemaphore(const common::FrameTracker &ft) const
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    {
        star::command_order::GetPassInfo get{m_commandBuffer};
        m_cmdBus->submit(get);
        signalValue = get.getReply().get().currentSignalValue;
        semaphore = get.getReply().get().signaledSemaphore;
    }

    const uint64_t frameCount = ft.getCurrent().getNumTimesFrameProcessed();
    if (frameCount == signalValue)
    {

        auto result =
            m_device.waitSemaphores(vk::SemaphoreWaitInfo().setValues(frameCount).setSemaphores(semaphore), UINT64_MAX);

        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to wait for timeline semaphores");
        }
    }
}

void HeadlessRenderer::recordCommandBuffer(StarCommandBuffer &commandBuffer, const common::FrameTracker &ft,
                                           const uint64_t &frameIndex)
{
    waitForSemaphore(ft);

    this->DefaultRenderer::recordCommandBuffer(commandBuffer, ft, frameIndex);
}

void HeadlessRenderer::prepRender(common::IDeviceContext &c)
{
    this->DefaultRenderer::prepRender(c);

    auto &context = static_cast<star::core::device::DeviceContext &>(c);
    m_cmdBus = &context.getCmdBus();
    m_device = context.getDevice().getVulkanDevice();
    m_imgMgr = &context.getGraphicsManagers().imageManager;

    m_prepScheme.resize(context.getFrameTracker().getSetup().getNumFramesInFlight(), pre_pass::DoNothing{});
    m_postScheme.resize(context.getFrameTracker().getSetup().getNumFramesInFlight(), post_pass::DoNothing{});

    // create timeline semaphores to use
    m_timelineSemaphores = CreateSemaphores(context.getEventBus(), context.getFrameTracker());
}

vk::Semaphore HeadlessRenderer::submitBuffer(star::StarCommandBuffer &buffer,
                                             const star::common::FrameTracker &frameTracker,
                                             std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                             std::vector<vk::Semaphore> dataSemaphores,
                                             std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                             std::vector<std::optional<uint64_t>> previousSignaledValues,
                                             star::StarQueue &queue)
{
    const size_t ii = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
    assert(m_cmdBus != nullptr);

    vk::SemaphoreSubmitInfo waitInfo[8];
    uint8_t waitInfoCount{0};

    vk::Semaphore mySemaphore{VK_NULL_HANDLE};
    uint64_t mySemaphoreSignalValue{0};
    {
        auto cmd = star::command_order::GetPassInfo{m_commandBuffer};
        m_cmdBus->submit(cmd);
        mySemaphore = cmd.getReply().get().signaledSemaphore;
        mySemaphoreSignalValue = cmd.getReply().get().toSignalValue;

        assert(cmd.getReply().get().edges != nullptr &&
               "No neighbor command buffers were registered. At least one is expected");
        assert(cmd.getReply().get().edges->size() + dataWaitPoints.size() < 8 &&
               "Static size container for wait semaphore info only expects a max of 8");
        for (const auto &edge : *cmd.getReply().get().edges)
        {
            if (edge.consumer == m_commandBuffer)
            {
                auto nCmd = star::command_order::GetPassInfo{edge.producer};
                m_cmdBus->submit(nCmd);

                waitInfo[waitInfoCount]
                    .setSemaphore(nCmd.getReply().get().signaledSemaphore)
                    .setValue(nCmd.getReply().get().toSignalValue)
                    .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

                waitInfoCount++;
            }
        }
    }

    assert(dataSemaphores.size() == dataWaitPoints.size());
    for (size_t i{0}; i < dataWaitPoints.size(); i++)
    {
        waitInfo[waitInfoCount + i]
            .setSemaphore(dataSemaphores[i])
            .setValue(previousSignaledValues[i].has_value() ? previousSignaledValues[i].value() : 0)
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
    }
    waitInfoCount += static_cast<uint8_t>(dataWaitPoints.size());

    vk::Semaphore binarySemaphore{buffer.getCompleteSemaphores()[ii]};
    const vk::SemaphoreSubmitInfo signalInfo[2]{
        vk::SemaphoreSubmitInfo()
            .setSemaphore(mySemaphore)
            .setValue(mySemaphoreSignalValue)
            .setStageMask(vk::PipelineStageFlagBits2::eAllCommands),
        vk::SemaphoreSubmitInfo().setSemaphore(binarySemaphore).setStageMask(vk::PipelineStageFlagBits2::eAllCommands)};
    const uint8_t signalInfoCount{2};

    queue.getVulkanQueue().submit2(vk::SubmitInfo2()
                                       .setPWaitSemaphoreInfos(waitInfo)
                                       .setWaitSemaphoreInfoCount(waitInfoCount)
                                       .setCommandBufferInfos(vk::CommandBufferSubmitInfo().setCommandBuffer(
                                           buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex())))
                                       .setPSignalSemaphoreInfos(signalInfo)
                                       .setSignalSemaphoreInfoCount(signalInfoCount));

    return binarySemaphore;
}
} // namespace star::core::renderer