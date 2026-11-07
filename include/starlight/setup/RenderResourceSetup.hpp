#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "structs/QueueFamilyOwnership.hpp"

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <future>
#include <string>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::core::device
{
class DeviceContext;
}

namespace star::setup
{
/// @brief One-shot GPU work a render phase performs before it samples a resource.
/// @details Work is split into two phases. prepRender() (called from the object's frameUpdate) does everything that
/// needs the device context: resolving contribution futures, caching each dispatch's built pipeline, and polling
/// release gates. record() then only touches Vulkan handles, so the recording path has no device dependency.
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
        /// @brief Debug/introspection metadata: handle-addressed resources this dispatch works with.
        /// @details Resources without a handle (worker-internal staging) are not listed; the descriptor set is the
        /// authority for what is actually bound.
        std::vector<Handle> resourceBindings{};
        /// @brief Opaque payload forwarded as compute-stage push constants.
        std::vector<std::byte> pushConstants{};
        std::array<uint32_t, 3> dispatchDims{1, 1, 1};

        /// @brief Vulkan pipeline resolved by prepare(); record() binds it without touching the pipeline manager.
        vk::Pipeline builtPipeline{VK_NULL_HANDLE};

        /// @brief Resolves the built pipeline into `builtPipeline`. Called from prepRender only.
        /// @return False while the pipeline manager has not finished building the pipeline.
        bool prepare(const core::device::DeviceContext &context);

        /// @brief Binds the cached pipeline, descriptor set and push constants, then dispatches.
        void record(vk::CommandBuffer commandBuffer) const;
    };

    // Add further engine primitives (buffer copies, clear operations, ...) here as consumers need them.
    using Step = std::variant<std::monostate, ImageLayoutChange, ComputeDispatch>;

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

    /// @brief One resource's complete preparation: the work and the cleanup/gate coupled to it.
    /// @details A definition is published as a whole -- synchronously by the composer, or later by a worker through a
    /// promise. Its steps record in definition order, then its release is tracked until the gate passes.
    struct ResourcePrepDefinition
    {
        std::vector<Step> steps{};
        PendingRelease release{};
    };

    /// @brief One-shot completion gate: plain bool, caller-free.
    /// @details True once every definition has been recorded. Consumers that sample the resource can gate uniformly
    /// on isDone().
    bool done = false;
    /// @brief Definitions in recording order, one future per resource.
    /// @details Only the composer touches this vector; workers only set their promise, so no lock is needed. Each
    /// future is resolved in prepRender and its steps recorded exactly once. Composed through Builder::add().
    /// Cleared and released as soon as the recording pass completes.
    std::vector<std::future<std::expected<ResourcePrepDefinition, std::string>>> definitions{};

    /// @brief Composes a setup and pre-sizes its per-frame working storage.
    /// @details Define every resource before calling build(). build() moves the definitions into the setup and
    /// reserves the interpreter's working vectors from the definition count, so prepRender()/record() perform no heap
    /// allocation. Adding definitions after build() is not supported.
    class Builder
    {
      public:
        /// @brief Adds a definition that is fully known now. Its future is ready immediately.
        Builder &add(ResourcePrepDefinition definition);
        /// @brief Adds a definition a worker publishes later through its promise, or reports as failed.
        Builder &add(std::future<std::expected<ResourcePrepDefinition, std::string>> definition);

        [[nodiscard]] RenderResourceSetup build();

      private:
        std::vector<std::future<std::expected<ResourcePrepDefinition, std::string>>> m_definitions{};
    };

    /// @brief True once every definition has been recorded (or there were none).
    [[nodiscard]] bool isDone() const noexcept
    {
        return done;
    }

    /// @brief True once recording happened and every release has drained; the setup owns nothing and can be dropped.
    [[nodiscard]] bool isFullyDrained() const noexcept
    {
        return done && m_resolvedDefinitions.empty() && m_liveReleases.empty();
    }

    /// @brief Per-frame preparation. Resolves contribution futures, caches built pipelines, and polls release gates.
    /// @details Never blocks. Called from the object's frameUpdate, before this frame's recording pass.
    void prepRender(core::device::DeviceContext &context);

    /// @brief Records every resolved step in one pass and stamps the recording-submission gate onto the releases.
    /// @details Device-free: all pipeline handles were cached by prepRender. Does nothing until prepRender has reported
    /// every definition resolved and recordable.
    void record(vk::CommandBuffer commandBuffer, const core::graphics::SemaphoreInfo &recordingSubmissionGate);

    /// @brief Teardown backstop: resolves any ready definitions, host-waits every registered gate, then drains.
    /// @details Load-bearing, not tidiness: an undrained release is cleaned up without explicit device cleanup. A
    /// definition still unfilled at teardown owns nothing yet; its producer must publish (and be cleaned up) before
    /// the device goes away.
    void cleanupRender(core::device::DeviceContext &context);

  private:
    /// @brief Moves every populated definition into m_resolvedDefinitions. Render-thread only; no blocking.
    /// @return True when every registered definition has been resolved (or failed), i.e. the whole setup can record.
    bool tryResolveDefinitions();
    /// @brief Drains every live release whose gate has been observed passed. Non-destructive (counter read, not wait).
    void pollPendingReleases(core::device::DeviceContext &context);

    /// @brief Definitions whose future resolved but whose steps have not been recorded yet.
    std::vector<ResourcePrepDefinition> m_resolvedDefinitions{};
    /// @brief Recorded releases awaiting their gate. Render-thread only.
    std::vector<PendingRelease> m_liveReleases{};
    /// @brief Set by prepRender once every contribution future has been consumed.
    bool m_allDefinitionsResolved{false};
    /// @brief Set by prepRender once every dispatch step has a built pipeline available.
    bool m_readyToRecord{false};
};
} // namespace star::setup
