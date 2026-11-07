#include "starlight/setup/RenderResourceSetup.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/device/managers/Pipeline.hpp"
#include "core/logging/LoggingFactory.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <type_traits>
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

/// @brief Caches every dispatch step's built pipeline. Called from prepRender (device available).
/// @return False while at least one pipeline manager record has not finished building.
bool PrepareSteps(std::vector<RenderResourceSetup::Step> &steps, const core::device::DeviceContext &context)
{
    for (auto &step : steps)
    {
        if (auto *dispatch = std::get_if<RenderResourceSetup::ComputeDispatch>(&step))
        {
            if (!dispatch->prepare(context))
                return false;
        }
    }

    return true;
}

void RecordSteps(const std::vector<RenderResourceSetup::Step> &steps, vk::CommandBuffer commandBuffer)
{
    for (const auto &step : steps)
    {
        std::visit(
            [&](const auto &value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (!std::is_same_v<T, std::monostate>)
                    value.record(commandBuffer);
            },
            step);
    }
}

void StampRelease(RenderResourceSetup::PendingRelease &release,
                  const core::graphics::SemaphoreInfo &recordingSubmissionGate)
{
    if (!release.gateOnRecordingSubmission)
        return;

    release.gateOnRecordingSubmission = false;

    // Only a timeline gate with a real value is observable. A binary semaphore or an untriggered pass (null handle /
    // value 0) cannot be polled; leave the release ungated and let the teardown backstop wait for device idleness
    // instead of risking a premature free.
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

bool RenderResourceSetup::ComputeDispatch::prepare(const core::device::DeviceContext &context)
{
    // Manager::get throws for a stale/unfilled handle; turn that into "not ready" rather than escaping frameUpdate.
    try
    {
        const auto *record = context.getPipelineManager().get(pipeline);
        if (record == nullptr || !record->builtPipeline.isRenderReady())
            return false;

        builtPipeline = record->builtPipeline.getVulkanPipeline();
        return true;
    }
    catch (const std::exception &e)
    {
        core::logging::warning("RenderResourceSetup: pipeline lookup failed: ", e.what());
        return false;
    }
}

void RenderResourceSetup::ComputeDispatch::record(vk::CommandBuffer commandBuffer) const
{
    assert(builtPipeline != VK_NULL_HANDLE && "ComputeDispatch::record called before prepare() resolved the pipeline");

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, builtPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout, 0, 1, &descriptorSet, 0, nullptr);

    if (!pushConstants.empty())
    {
        commandBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eCompute, 0,
                                    static_cast<uint32_t>(pushConstants.size()), pushConstants.data());
    }

    commandBuffer.dispatch(dispatchDims[0], dispatchDims[1], dispatchDims[2]);
}

RenderResourceSetup::Builder &RenderResourceSetup::Builder::add(ResourcePrepDefinition definition)
{
    std::promise<std::expected<ResourcePrepDefinition, std::string>> ready;
    m_definitions.push_back(ready.get_future());
    ready.set_value(std::expected<ResourcePrepDefinition, std::string>{std::move(definition)});

    return *this;
}

RenderResourceSetup::Builder &RenderResourceSetup::Builder::add(
    std::future<std::expected<ResourcePrepDefinition, std::string>> definition)
{
    m_definitions.push_back(std::move(definition));
    return *this;
}

RenderResourceSetup RenderResourceSetup::Builder::build()
{
    RenderResourceSetup setup;
    setup.definitions = std::move(m_definitions);

    // One resolved definition and at most one live release per definition. Reserving here keeps the interpreter free
    // of heap allocation once the setup is in the frame loop.
    const std::size_t definitionCount = setup.definitions.size();
    setup.m_resolvedDefinitions.reserve(definitionCount);
    setup.m_liveReleases.reserve(definitionCount);

    return setup;
}

void RenderResourceSetup::prepRender(core::device::DeviceContext &context)
{
    if (!done)
    {
        m_allDefinitionsResolved = tryResolveDefinitions();

        if (m_allDefinitionsResolved)
        {
            // Pipeline builds are asynchronous; nothing records until every dispatch has a built pipeline available.
            m_readyToRecord = true;
            for (auto &definition : m_resolvedDefinitions)
            {
                if (!PrepareSteps(definition.steps, context))
                {
                    m_readyToRecord = false;
                    break;
                }
            }
        }
    }

    pollPendingReleases(context);
}

void RenderResourceSetup::record(vk::CommandBuffer commandBuffer,
                                 const core::graphics::SemaphoreInfo &recordingSubmissionGate)
{
    if (done || !m_allDefinitionsResolved || !m_readyToRecord)
        return;

    // Definitions record in composition order.
    for (auto &definition : m_resolvedDefinitions)
    {
        RecordSteps(definition.steps, commandBuffer);
        StampRelease(definition.release, recordingSubmissionGate);
        m_liveReleases.push_back(std::move(definition.release));
    }

    // Recording is the setup's last use of the definitions and their futures. Release that storage (including
    // capacity) now instead of keeping it alive for the object's lifetime; only the release gates remain to poll.
    m_resolvedDefinitions.clear();
    m_resolvedDefinitions.shrink_to_fit();
    definitions.clear();
    definitions.shrink_to_fit();

    done = true;
}

bool RenderResourceSetup::tryResolveDefinitions()
{
    bool allResolved = true;

    for (auto &slot : definitions)
    {
        // get() invalidates a unique future, so a resolved definition is skipped on later passes.
        if (!slot.valid())
            continue;

        if (slot.wait_for(std::chrono::seconds{0}) != std::future_status::ready)
        {
            allResolved = false;
            continue;
        }

        try
        {
            auto result = slot.get();
            if (result.has_value())
            {
                m_resolvedDefinitions.push_back(std::move(*result));
            }
            else
            {
                // Fail soft: log the reason and treat the definition as resolved-but-empty. done still flips so the
                // object renders with a blank resource; consumers are expected to check what they sample.
                core::logging::warning("RenderResourceSetup: a resource contribution failed: ", result.error(),
                                       ". Recording without it.");
            }
        }
        catch (const std::future_error &e)
        {
            core::logging::warning("RenderResourceSetup: a contribution producer was destroyed without publishing: ",
                                   e.what(), ". Recording without it.");
        }
    }

    return allResolved;
}

void RenderResourceSetup::pollPendingReleases(core::device::DeviceContext &context)
{
    if (m_liveReleases.empty())
    {
        // Before recording, keep the reservation so the record pass does not allocate. Once the setup has recorded,
        // release the backing storage instead of holding it for the object's lifetime.
        if (done && m_liveReleases.capacity() != 0)
            m_liveReleases.shrink_to_fit();
        return;
    }

    vk::Device device = context.getDevice().getVulkanDevice();

    const auto isPassed = [device](const PendingRelease &release) {
        if (!release.keepAlive)
            return true;

        // A release with no timeline gate cannot be observed; it is drained only by the teardown backstop, which
        // waits for device idleness first.
        if (release.timelineSemaphore == VK_NULL_HANDLE)
            return false;

        // Non-destructive monotonic counter read, unlike command-buffer fences which the frame loop waits and resets.
        return device.getSemaphoreCounterValue(release.timelineSemaphore) >= release.waitForValue;
    };

    for (auto it = m_liveReleases.begin(); it != m_liveReleases.end();)
    {
        if (!isPassed(*it))
        {
            ++it;
            continue;
        }

        if (it->keepAlive)
            it->keepAlive(context);

        it = m_liveReleases.erase(it);
    }
}

void RenderResourceSetup::cleanupRender(core::device::DeviceContext &context)
{
    vk::Device device = context.getDevice().getVulkanDevice();

    // Pick up every contribution published since the last record. A slot still unfilled owns nothing from the setup's
    // perspective; its producer is responsible for it and must publish before teardown.
    tryResolveDefinitions();

    // A definition resolved after the last record was never recorded, but still owns its transient resources.
    for (auto &definition : m_resolvedDefinitions)
        m_liveReleases.push_back(std::move(definition.release));
    m_resolvedDefinitions.clear();

    // A release whose gate was never stamped cannot be observed. If one exists, fall back to device idleness rather
    // than risk destroying a resource the GPU still reads.
    const bool hasUnobservableRelease =
        std::any_of(m_liveReleases.begin(), m_liveReleases.end(), [](const PendingRelease &release) {
            return release.keepAlive && release.timelineSemaphore == VK_NULL_HANDLE;
        });
    if (hasUnobservableRelease)
        context.waitIdle();

    // Host-wait every live gate before draining: a resource destroyed while the GPU still reads it skips explicit
    // device cleanup.
    for (auto &release : m_liveReleases)
    {
        if (!release.keepAlive || release.timelineSemaphore == VK_NULL_HANDLE)
            continue;

        (void)device.waitSemaphores(
            vk::SemaphoreWaitInfo().setSemaphores(release.timelineSemaphore).setValues(release.waitForValue),
            UINT64_MAX);
    }

    for (auto &release : m_liveReleases)
    {
        if (release.keepAlive)
            release.keepAlive(context);
    }

    // Teardown: release every backing buffer; nothing outlives the setup.
    m_liveReleases.clear();
    m_liveReleases.shrink_to_fit();
    m_resolvedDefinitions.clear();
    m_resolvedDefinitions.shrink_to_fit();
    definitions.clear();
    definitions.shrink_to_fit();

    done = true;
}
} // namespace star::setup
