#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"

#include <star_common/Handle.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star
{
namespace core::device
{
class DeviceContext;
}
namespace core::device::manager
{
class Image;
class Buffer;
class Shader;
class Pipeline;
} // namespace core::device::manager

/// @brief Outcome of one readiness check.
/// @details `Pending` means the producer has not published yet; `Failed` means it published an unusable result or the
/// condition itself is invalid. A condition that blocks internally never returns `Pending`.
enum class ReadinessState : std::uint8_t
{
    Ready,
    Pending,
    Failed
};

/// @brief Texture published by the Image manager.
struct ImageReadiness
{
    core::device::manager::Image *manager{nullptr};
    [[nodiscard]] ReadinessState check(const Handle &resource) const;
};

/// @brief Buffer published by the Buffer manager.
struct BufferReadiness
{
    core::device::manager::Buffer *manager{nullptr};
    [[nodiscard]] ReadinessState check(const Handle &resource) const;
};

/// @brief Legacy ManagerRenderResource buffer (engine vertex/instance/uniform buffers).
struct RenderResourceBufferReadiness
{
    Handle deviceID{};
    [[nodiscard]] ReadinessState check(const Handle &resource) const;
};

/// @brief Shader compiled by the Shader manager. The compile worker publishes the record-level future, so this check
/// is pump-free.
struct ShaderReadiness
{
    core::device::manager::Shader *manager{nullptr};
    [[nodiscard]] ReadinessState check(const Handle &resource) const;
};

/// @brief Pipeline built by the Pipeline manager.
/// @details The build is queued from the main-thread completion path, so a blocking wait must pump complete messages
/// to let the shader-completion trigger queue the build.
struct PipelineReadiness
{
    core::device::manager::Pipeline *manager{nullptr};
    core::device::DeviceContext *context{nullptr};
    [[nodiscard]] ReadinessState check(const Handle &resource) const;
};

/// @brief One strongly typed one-time readiness condition for a resource handle.
struct ReadinessCondition
{
    Handle resource{};
    std::variant<ImageReadiness, BufferReadiness, RenderResourceBufferReadiness, ShaderReadiness, PipelineReadiness>
        readiness;

    /// @brief Set for transfer-produced resources: the GPU timeline the owning pass must wait on.
    /// @details The CPU publication check is expressed by `readiness`; this is the matching GPU-side wait.
    std::optional<core::graphics::SemaphoreInfo> timelineWait;
    vk::PipelineStageFlags waitStages =
        vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;

    [[nodiscard]] ReadinessState check() const;
};

/// @brief Poll pending conditions until all are Ready, one fails, or the deadline passes.
/// @param outStates Optional per-condition results from the final evaluation pass; resized to the condition count.
/// @return `Ready` when every condition is ready, `Failed` on the first failure (or a throwing check), `Pending` when
/// the deadline passed with work outstanding.
[[nodiscard]] ReadinessState EvaluateReadiness(const std::vector<ReadinessCondition> &conditions,
                                               std::chrono::steady_clock::time_point deadline,
                                               std::vector<ReadinessState> *outStates = nullptr);

/// @brief Blocking convenience: true only when every condition becomes ready before the timeout.
[[nodiscard]] bool WaitForReadiness(const std::vector<ReadinessCondition> &conditions,
                                    std::chrono::milliseconds timeout);
} // namespace star
