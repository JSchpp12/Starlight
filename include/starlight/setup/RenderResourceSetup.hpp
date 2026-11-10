#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "structs/QueueFamilyOwnership.hpp"
#include "wrappers/graphics/StarSemaphore.hpp"

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
/// @brief One transfer-completion wait a pass must observe before it samples/uses a resource.
struct TransferWait
{
    Handle key;
    star::StarSemaphore semaphore;
    vk::PipelineStageFlags stages =
        vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;
};

/// @brief One-shot GPU work a render phase records before it samples a resource.
/// @details Preparation is separate from recording. A `PrepRequest` carries only the device-side prepare
/// callback; the object that owns the material runs it and hands the prepared definition to the pass's
/// RenderResourceSetup, which records it inline before the pass's draws.
struct RenderResourceSetup
{
    /// @brief Transition an image between layouts.
    /// @details Holds the raw image rather than a Handle: the only producer of this step for a freshly created
    /// resource is the transfer thread, which holds the vk::Image but cannot know the manager-assigned Handle.
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

        /// @brief Built pipeline cache, filled by ready() once the pipeline manager reports it built.
        std::shared_ptr<vk::Pipeline> builtPipeline = std::make_shared<vk::Pipeline>(VK_NULL_HANDLE);

        /// @brief Resolves the built pipeline when the pipeline manager reports it ready.
        /// @return False while the asynchronous pipeline build has not finished.
        [[nodiscard]] bool ready(const core::device::DeviceContext &context) const;

        /// @brief Binds the resolved pipeline, descriptor set and push constants, then dispatches.
        void record(vk::CommandBuffer commandBuffer) const;
    };

    // Add further engine primitives (buffer copies, clear operations, ...) here as consumers need them.
    /// @brief One record-only command with an optional per-frame readiness check.
    struct Step
    {
        /// @brief Resolves any asynchronous dependency (e.g. a pipeline build). Empty means always ready.
        std::function<bool(const core::device::DeviceContext &)> ready{};
        /// @brief Device-free recording.
        std::function<void(vk::CommandBuffer)> record{};
    };

    /// @brief Wraps a layout transition as a Step.
    [[nodiscard]] static Step makeStep(ImageLayoutChange change);
    /// @brief Wraps a compute dispatch as a Step. The pipeline must be resolved by the definition's prepare callback.
    [[nodiscard]] static Step makeStep(std::shared_ptr<ComputeDispatch> dispatch);

    /// @brief A transient resource released once a timeline gate is observed passed.
    /// @details The gate is a generic {semaphore, value} registration, so any timeline source can gate a release.
    /// The one value that cannot be known at composition time -- the submission that records the setup -- is marked
    /// gateOnRecordingSubmission and stamped when the definition that owns the release is recorded. Registered
    /// semaphores must outlive their release: register engine-long-lived timelines (the transfer-service shared
    /// timeline, a render phase timeline), never a request-scoped semaphore.
    /// @note A PendingRelease destroyed without draining skips its cleanup, so cleanupRender() must run while the
    /// device is alive.
    struct PendingRelease
    {
        vk::Semaphore timelineSemaphore{};
        uint64_t waitForValue{};
        /// @brief Stamp {semaphore, value} from the recording submission when this release is recorded.
        bool gateOnRecordingSubmission{};
        /// @brief Cleanup for the resource this release owns, invoked once the gate passes (or at teardown).
        /// @details The device context is supplied by the interpreter, so the producer only captures the resource
        /// identity (e.g. a manager handle) and can reach the matching manager through the context. Empty when the
        /// definition owns nothing.
        std::function<void(core::device::DeviceContext &)> keepAlive{};
    };

    /// @brief One resource's prepared work: record-only steps and the cleanup/gate coupled to them.
    struct ResourcePrepDefinition
    {
        std::vector<Step> steps{};
        PendingRelease release{};
        /// @brief Set by prepare() when every step's async dependency is resolved for this frame.
        bool ready{false};
        /// @brief Set once the steps were recorded into the pass's command buffer.
        bool recorded{false};

        [[nodiscard]] bool hasWork() const noexcept;
        /// @brief Resolves the steps' asynchronous dependencies for this frame (e.g. a pipeline build).
        void prepare(const core::device::DeviceContext &context);
        /// @brief Records the steps when prepare() found them ready, stamps the release, and marks recorded.
        [[nodiscard]] bool record(vk::CommandBuffer commandBuffer,
                                  const core::graphics::SemaphoreInfo &recordingSubmissionGate);
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
    void add(ResourcePrepDefinition definition);

    /// @brief True when any prepared work is owned.
    [[nodiscard]] bool hasWork() const noexcept
    {
        return !definitions.empty();
    }

    /// @brief True once every owned definition has been recorded (or there were none).
    [[nodiscard]] bool isDone() const noexcept;

    /// @brief Resolves each definition's per-frame readiness (called from the phase's frameUpdate).
    void prepare(const core::device::DeviceContext &context);

    /// @brief Records every ready, not-yet-recorded definition in one pass.
    void record(vk::CommandBuffer commandBuffer, const core::graphics::SemaphoreInfo &recordingSubmissionGate);

    /// @brief Non-blocking per-frame drain: runs passed releases and erases fully drained definitions.
    /// @details Call from the owning phase's frameUpdate after prepare(). Without it, a recorded definition's
    /// keepAlive (e.g. a per-make descriptor set) is only ever run by the teardown backstop.
    void pollReleases(core::device::DeviceContext &context);

    /// @brief Teardown backstop: host-waits every release gate, runs each keepAlive, and clears.
    void cleanupRender(core::device::DeviceContext &context);
};
} // namespace star::setup
