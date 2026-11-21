#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "structs/QueueFamilyOwnership.hpp"

#include <star_common/Handle.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::core::device
{
class DeviceContext;
}

namespace star::setup
{
/// @brief Stable identity for one prepared definition within a pass's setup.
using DefinitionId = std::uint64_t;
inline constexpr DefinitionId InvalidDefinitionId = 0;

/// @brief One transfer-completion wait a pass must observe before it samples/uses a resource.
struct TransferWait
{
    Handle key;
    /// Semaphore-manager record of the transfer's timeline; resolved to the live semaphore state on submission.
    Handle semaphoreRecord;
    vk::PipelineStageFlags stages =
        vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
};

/// @brief One-shot GPU work a render phase records before it samples a resource.
struct RenderResourceSetup
{
    using DefinitionId = star::setup::DefinitionId;
    static constexpr DefinitionId InvalidDefinitionId = star::setup::InvalidDefinitionId;

    /// @brief Transition an image between layouts.
    struct ImageLayoutChange
    {
        vk::Image image{};
        vk::ImageLayout from = vk::ImageLayout::eUndefined;
        vk::ImageLayout to = vk::ImageLayout::eUndefined;
        vk::ImageSubresourceRange range{};
        /// @brief Queue-family ownership transfer parameters. Both ignored = a plain layout transition.
        QueueFamilyOwnership ownership{};

        /// @brief Records the layout transition and any ownership transfer. Stage/access masks are derived from the
        /// layouts; the composed src/dst pair is used as-is (a release and its acquire carry the same pair).
        void record(vk::CommandBuffer commandBuffer) const;
    };

    /// @brief Dispatch a compute pipeline whose descriptor set binds the setup's own resources.
    struct ComputeDispatch
    {
        /// @brief Pipeline handle owned by the engine's pipeline manager.
        Handle pipeline{};
        /// @brief Descriptor set binding the setup's staging resource and destination resource.
        vk::DescriptorSet descriptorSet{};
        /// @brief Pipeline layout the set and push constants are bound through.
        vk::PipelineLayout layout{};
        /// @brief Opaque payload forwarded as compute-stage push constants.
        std::vector<std::byte> pushConstants{};
        std::array<uint32_t, 3> dispatchDims{1, 1, 1};

        /// @brief Device used to resolve the built pipeline at record time. Set by the producer.
        /// @details The producer/commit gate must guarantee the pipeline is built before the definition is recorded.
        const core::device::DeviceContext *context{nullptr};

        /// @brief Resolves the built pipeline from the manager and issues the dispatch.
        void record(vk::CommandBuffer commandBuffer) const;
    };

    // Add further engine primitives (buffer copies, clear operations, ...) here as consumers need them.
    /// @brief One record-only command.
    struct Step
    {
        /// @brief Device-free recording.
        std::function<void(vk::CommandBuffer)> record{};
    };

    /// @brief Wraps a layout transition as a Step.
    [[nodiscard]] static Step makeStep(ImageLayoutChange change);
    /// @brief Wraps a compute dispatch as a Step. The dispatch must carry a DeviceContext.
    [[nodiscard]] static Step makeStep(std::shared_ptr<ComputeDispatch> dispatch);

    /// @brief A transient resource released once a timeline gate is observed passed.

    struct PendingRelease
    {
        vk::Semaphore timelineSemaphore{};
        uint64_t waitForValue{};
        /// @brief Stamp {semaphore, value} from the recording submission when this release is recorded.
        bool gateOnRecordingSubmission{};
        /// @brief Cleanup for the resource this release owns, invoked once the gate passes (or at teardown).
        std::function<void(core::device::DeviceContext &)> keepAlive{};
    };

    /// @brief One resource's prepared work: record-only steps and the cleanup/gate coupled to them.
    struct ResourcePrepDefinition
    {
        std::vector<Step> steps{};
        PendingRelease release{};
        /// @brief Stable identity assigned by `add`; survives `pollReleases` erasure.
        DefinitionId id{InvalidDefinitionId};
        /// @brief Set once the steps were recorded into the pass's command buffer.
        bool recorded{false};

        [[nodiscard]] bool hasWork() const noexcept;
        /// @brief Records the steps, stamps the release, and marks recorded.
        /// @details The caller must have ensured every step's dependencies are ready (via the readiness conditions
        /// registered with the commit gate / CPU-wait system).
        [[nodiscard]] bool record(vk::CommandBuffer commandBuffer,
                                  const core::graphics::SemaphoreSyncPoint &recordingSubmissionGate);
        /// @brief Non-blocking release drain: runs the keepAlive once the recorded gate has been observed passed.
        /// @return True when recorded and the release is consumed; false while work or the gate remains.
        [[nodiscard]] bool tryDrainRelease(core::device::DeviceContext &context);
        /// @brief Runs the release's keepAlive assuming the caller has already made the device idle, then clears it.
        /// @details Used when a definition will never be recorded (failed swap). Unlike cleanupRender it never calls
        /// DeviceContext::waitIdle(), which would tear down the task manager mid-run.
        void releaseAssumeDeviceIdle(core::device::DeviceContext &context);
        /// @brief Waits for the release gate (or device idleness) and runs its keepAlive.
        void cleanupRender(core::device::DeviceContext &context);
    };

    /// @brief The one-time device work needed before a resource can record.
    /// @details `prepare` fills the caller-provided definition; it may block on a producer's futures. The
    /// prepared definition is owned by the consuming pass's RenderResourceSetup, never by the producer.
    struct PrepRequest
    {
        std::function<void(const core::device::DeviceContext &, ResourcePrepDefinition &)> prepare{};
    };

    /// @brief Prepared definitions in recording order.
    std::vector<ResourcePrepDefinition> definitions;

    /// @brief Adds prepared work. Empty definitions are dropped.
    /// @return The definition's stable id, or InvalidDefinitionId when it was dropped.
    [[nodiscard]] DefinitionId add(ResourcePrepDefinition definition);

    /// @brief Abandon an unrecorded definition (e.g. a rolled-back swap).
    /// @details Stamps the release gate from @p gate when observable, clears the recording-submission gate, and marks
    /// the definition recorded so `isDone()` unblocks and `pollReleases` can drain its keepAlive later.
    void abandon(DefinitionId id, const core::graphics::SemaphoreSyncPoint &gate);

    /// @brief True when any prepared work is owned.
    [[nodiscard]] bool hasWork() const noexcept
    {
        return !definitions.empty();
    }

    /// @brief True once every owned definition has been recorded (or there were none).
    [[nodiscard]] bool isDone() const noexcept;

    /// @brief Records every not-yet-recorded definition in one pass.
    void record(vk::CommandBuffer commandBuffer, const core::graphics::SemaphoreSyncPoint &recordingSubmissionGate);

    /// @brief Non-blocking per-frame drain: runs passed releases and erases fully drained definitions.
    /// @details Call from the owning phase's frameUpdate. Without it, a recorded definition's keepAlive (e.g. a
    /// per-make descriptor set) is only ever run by the teardown backstop.
    void pollReleases(core::device::DeviceContext &context);

    /// @brief Teardown backstop: host-waits every release gate, runs each keepAlive, and clears.
    void cleanupRender(core::device::DeviceContext &context);

  private:
    DefinitionId m_nextDefinitionId{InvalidDefinitionId};
};
} // namespace star::setup
