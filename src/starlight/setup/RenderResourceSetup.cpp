#include "starlight/setup/RenderResourceSetup.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/logging/LoggingFactory.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace star::setup
{
namespace
{
vk::PipelineStageFlags2 StageForLayout(vk::ImageLayout layout) noexcept
{
    switch (layout)
    {
    case vk::ImageLayout::eUndefined:
        return vk::PipelineStageFlagBits2::eNone;
    case vk::ImageLayout::eGeneral:
        return vk::PipelineStageFlagBits2::eComputeShader;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::PipelineStageFlagBits2::eFragmentShader;
    case vk::ImageLayout::eTransferSrcOptimal:
    case vk::ImageLayout::eTransferDstOptimal:
        return vk::PipelineStageFlagBits2::eTransfer;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
    default:
        return vk::PipelineStageFlagBits2::eAllCommands;
    }
}

vk::AccessFlags2 AccessForLayout(vk::ImageLayout layout) noexcept
{
    switch (layout)
    {
    case vk::ImageLayout::eUndefined:
    case vk::ImageLayout::ePresentSrcKHR:
        return vk::AccessFlagBits2::eNone;
    case vk::ImageLayout::eGeneral:
        return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::AccessFlagBits2::eShaderRead;
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::AccessFlagBits2::eTransferRead;
    case vk::ImageLayout::eTransferDstOptimal:
        return vk::AccessFlagBits2::eTransferWrite;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    default:
        return vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    }
}

void RecordSteps(const std::vector<RenderResourceSetup::Step> &steps, vk::CommandBuffer commandBuffer)
{
    for (const auto &step : steps)
    {
        if (step.record)
            step.record(commandBuffer);
    }
}

void StampRelease(RenderResourceSetup::PendingRelease &release,
                  const core::graphics::SemaphoreInfo &recordingSubmissionGate)
{
    if (!release.gateOnRecordingSubmission)
        return;

    release.gateOnRecordingSubmission = false;

    // Only a timeline gate with a real value is observable. A binary semaphore or an untriggered pass (null handle /
    // value 0) cannot be polled; leave the release ungated and let cleanupRender wait for device idleness instead of
    // risking a premature free.
    if (recordingSubmissionGate.semaphore == VK_NULL_HANDLE || recordingSubmissionGate.signalValue == 0)
        return;

    release.timelineSemaphore = recordingSubmissionGate.semaphore;
    release.waitForValue = recordingSubmissionGate.signalValue;
}
} // namespace

void RenderResourceSetup::ImageLayoutChange::record(vk::CommandBuffer commandBuffer) const
{
    const uint32_t dstQueueFamily = ownership.dst;

    const vk::ImageMemoryBarrier2 barrier = vk::ImageMemoryBarrier2()
                                                .setImage(image)
                                                .setOldLayout(from)
                                                .setNewLayout(to)
                                                .setSrcQueueFamilyIndex(ownership.src)
                                                .setDstQueueFamilyIndex(dstQueueFamily)
                                                .setSrcStageMask(StageForLayout(from))
                                                .setSrcAccessMask(AccessForLayout(from))
                                                .setDstStageMask(StageForLayout(to))
                                                .setDstAccessMask(AccessForLayout(to))
                                                .setSubresourceRange(range);

    commandBuffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barrier));
}

void RenderResourceSetup::ComputeDispatch::record(vk::CommandBuffer commandBuffer) const
{
    assert(context != nullptr && "ComputeDispatch requires a DeviceContext to resolve its pipeline");

    const auto *record = context->getPipelineManager().get(pipeline);
    assert(record != nullptr && record->isReady() &&
           "ComputeDispatch::record called before the pipeline was ready");
    const vk::Pipeline builtPipeline = record->builtPipeline.getVulkanPipeline();
    assert(builtPipeline != VK_NULL_HANDLE && "ComputeDispatch resolved a null pipeline");

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, builtPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, 1, &descriptorSet, 0, nullptr);

    if (!pushConstants.empty())
    {
        commandBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0,
                                    static_cast<uint32_t>(pushConstants.size()), pushConstants.data());
    }

    commandBuffer.dispatch(dispatchDims[0], dispatchDims[1], dispatchDims[2]);
}

RenderResourceSetup::Step RenderResourceSetup::makeStep(ImageLayoutChange change)
{
    Step step;
    step.record = [change = std::move(change)](vk::CommandBuffer commandBuffer) { change.record(commandBuffer); };
    return step;
}

RenderResourceSetup::Step RenderResourceSetup::makeStep(std::shared_ptr<ComputeDispatch> dispatch)
{
    Step step;
    step.record = [dispatch = std::move(dispatch)](vk::CommandBuffer commandBuffer) {
        assert(dispatch && "ComputeDispatch step requires a dispatch");
        dispatch->record(commandBuffer);
    };
    return step;
}

bool RenderResourceSetup::ResourcePrepDefinition::hasWork() const noexcept
{
    return !steps.empty() || release.gateOnRecordingSubmission || release.keepAlive ||
           release.timelineSemaphore != VK_NULL_HANDLE;
}

bool RenderResourceSetup::ResourcePrepDefinition::record(vk::CommandBuffer commandBuffer,
                                                         const core::graphics::SemaphoreInfo &recordingSubmissionGate)
{
    if (recorded)
        return true;

    RecordSteps(steps, commandBuffer);
    StampRelease(release, recordingSubmissionGate);
    recorded = true;
    return true;
}

void RenderResourceSetup::ResourcePrepDefinition::releaseAssumeDeviceIdle(core::device::DeviceContext &context)
{
    if (release.keepAlive)
    {
        release.keepAlive(context);
        release.keepAlive = {};
    }

    release.timelineSemaphore = VK_NULL_HANDLE;
    release.waitForValue = 0;
    release.gateOnRecordingSubmission = false;
    recorded = true;
}

void RenderResourceSetup::ResourcePrepDefinition::cleanupRender(core::device::DeviceContext &context)
{
    if (release.keepAlive)
    {
        vk::Device device = context.getDevice().getVulkanDevice();

        if (release.timelineSemaphore != VK_NULL_HANDLE)
        {
            (void)device.waitSemaphores(
                vk::SemaphoreWaitInfo().setSemaphores(release.timelineSemaphore).setValues(release.waitForValue),
                UINT64_MAX);
        }
    }

    releaseAssumeDeviceIdle(context);
}

bool RenderResourceSetup::ResourcePrepDefinition::tryDrainRelease(core::device::DeviceContext &context)
{
    if (!recorded)
        return false;

    if (!release.keepAlive)
        return true;

    // An unobservable gate cannot be polled. Leave it to cleanupRender(), which waits for device idleness instead.
    if (release.timelineSemaphore == VK_NULL_HANDLE || release.waitForValue == 0)
        return false;

    const vk::Device device = context.getDevice().getVulkanDevice();
    if (device.getSemaphoreCounterValue(release.timelineSemaphore) < release.waitForValue)
        return false;

    release.keepAlive(context);
    release.keepAlive = {};
    return true;
}

RenderResourceSetup::DefinitionId RenderResourceSetup::add(ResourcePrepDefinition definition)
{
    if (!definition.hasWork())
        return InvalidDefinitionId;

    definition.id = ++m_nextDefinitionId;
    const DefinitionId id = definition.id;
    definitions.push_back(std::move(definition));
    return id;
}

void RenderResourceSetup::abandon(DefinitionId id, const core::graphics::SemaphoreInfo &gate)
{
    if (id == InvalidDefinitionId)
        return;

    const auto it = std::find_if(definitions.begin(), definitions.end(),
                                 [id](const ResourcePrepDefinition &definition) { return definition.id == id; });
    if (it == definitions.end() || it->recorded)
        return;

    // Stamp an observable gate so pollReleases can drain the keepAlive; otherwise cleanupRender owns it at teardown.
    if (gate.semaphore != VK_NULL_HANDLE && gate.signalValue != 0)
    {
        it->release.timelineSemaphore = gate.semaphore;
        it->release.waitForValue = gate.signalValue;
    }
    it->release.gateOnRecordingSubmission = false;
    it->recorded = true;
}

bool RenderResourceSetup::isDone() const noexcept
{
    return std::all_of(definitions.begin(), definitions.end(),
                       [](const auto &definition) { return definition.recorded; });
}

void RenderResourceSetup::record(vk::CommandBuffer commandBuffer,
                                 const core::graphics::SemaphoreInfo &recordingSubmissionGate)
{
    for (auto &definition : definitions)
        (void)definition.record(commandBuffer, recordingSubmissionGate);
}

void RenderResourceSetup::pollReleases(core::device::DeviceContext &context)
{
    // Erase drained definitions so the setup does not grow without bound across repeated one-shot work (e.g. a
    // runtime material swap) and so per-make releases are actually run while the app is running.
    std::erase_if(definitions,
                  [&context](ResourcePrepDefinition &definition) { return definition.tryDrainRelease(context); });
}

void RenderResourceSetup::cleanupRender(core::device::DeviceContext &context)
{
    for (auto &definition : definitions)
        definition.cleanupRender(context);

    definitions.clear();
}
} // namespace star::setup
