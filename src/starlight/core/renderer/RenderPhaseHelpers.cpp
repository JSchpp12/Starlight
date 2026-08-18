#include "starlight/core/renderer/RenderPhaseHelpers.hpp"

#include "starlight/command/command_order/DeclarePass.hpp"
#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/DeviceContext.hpp"
#include "starlight/core/device/managers/Semaphore.hpp"
#include "starlight/core/device/system/event/ManagerRequest.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/core/renderer/EdgeSubmission.hpp"
#include "starlight/wrappers/graphics/StarCommandBuffer.hpp"
#include "starlight/wrappers/graphics/StarQueue.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace star::core::renderer
{
std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
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

void RegisterWithCommandOrder(const star::core::CommandBus &cmdBus, star::common::EventBus &evtBus,
                              star::core::device::manager::Queue &qm, star::Handle commandBuffer)
{
    auto *queue = star::core::helper::GetEngineDefaultQueue(evtBus, qm, star::Queue_Type::Tgraphics);
    assert(queue != nullptr && "Failed to acquire default engine queue");

    cmdBus.submit(star::command_order::DeclarePass{std::move(commandBuffer), queue->getParentQueueFamilyIndex()});
}

std::vector<star::StarRenderGroup> CreateRenderingGroups(star::core::device::DeviceContext &context,
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

void waitForTimelineSemaphore(const star::core::CommandBus &cmdBus, vk::Device device, star::Handle commandBuffer,
                              const star::common::FrameTracker &ft)
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    {
        star::command_order::GetPassInfo get{commandBuffer};
        cmdBus.submit(get);
        signalValue = get.getReply().get().currentSignalValue;
        semaphore = get.getReply().get().signaledSemaphore;
    }

    const uint64_t frameCount = ft.getCurrent().getNumTimesFrameProcessed();
    if (frameCount == signalValue)
    {
        assert(device != VK_NULL_HANDLE);
        auto result =
            device.waitSemaphores(vk::SemaphoreWaitInfo().setValues(frameCount).setSemaphores(semaphore), UINT64_MAX);

        if (result != vk::Result::eSuccess)
            STAR_THROW("Failed to wait for timeline semaphores");
    }
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride>
makeEdgeAwareSubmissionOverride(const star::core::CommandBus *cmdBus, const star::Handle *commandBuffer,
                                bool signalBinaryCompletion)
{
    star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride overrideFn =
        [cmdBus, commandBuffer, signalBinaryCompletion](
            star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
            std::vector<vk::Semaphore> *previousCommandBufferSemaphores, std::vector<vk::Semaphore> &dataSemaphores,
            std::vector<vk::PipelineStageFlags> &dataWaitPoints,
            std::vector<std::optional<uint64_t>> &previousSignaledValues, star::StarQueue &queue) -> vk::Semaphore {
        return submitEdgeAwarePass(*cmdBus, *commandBuffer, buffer, frameTracker, previousCommandBufferSemaphores,
                                   dataSemaphores, dataWaitPoints, previousSignaledValues, queue,
                                   signalBinaryCompletion);
    };

    return overrideFn;
}

std::optional<star::core::graphics::SemaphoreInfo> GetNeighborConsumerSyncInfo(const star::core::CommandBus &cmdBus,
                                                                               const star::Handle &myCommandBuffer)
{
    auto cmd = star::command_order::GetPassInfo{myCommandBuffer};
    cmdBus.submit(cmd);
    const auto &r = cmd.getReply().get();

    if (r.edges != nullptr)
    {
        for (const auto edge : *r.edges)
        {
            if (edge.producer == myCommandBuffer)
            {
                auto nCmd = star::command_order::GetPassInfo{edge.consumer};
                cmdBus.submit(nCmd);

                const auto &nr = nCmd.getReply().get();
                assert(nr.wasProcessedOnLastFrame != nullptr &&
                       "Neighbor last submission records was not provided by command_order service. This indicates a "
                       "bug in that service.");

                return star::core::graphics::SemaphoreInfo{
                    .signalValue = nr.currentSignalValue,
                    .semaphore = nr.signaledSemaphore,
                };
            }
        }
    }

    return std::nullopt;
}
} // namespace star::core::renderer
