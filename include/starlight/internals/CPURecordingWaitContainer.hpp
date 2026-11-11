#pragma once

#include <star_common/Handle.hpp>

#include <variant>
#include <vector>

namespace star::core::device::manager
{
class Image;
class Buffer;
} // namespace star::core::device::manager

namespace star
{
/// @brief Owns a pass's host-side readiness waits for transfer resources.
/// @details Each wait is one strongly typed readiness check stored in a variant -- no type erasure and no allocation.
/// The pass runs every registered wait on the recording thread before it records. This class only executes the host
/// waits; the pass owns the matching GPU timeline waits and drops a failed resource's entry itself.
class CPURecordingWaitContainer
{
  public:
    /// @brief Readiness check for a texture published by the Image manager.
    struct ImageReadiness
    {
        core::device::manager::Image *manager{nullptr};

        /// @brief Block until the record's future publishes, then validate the texture exists.
        [[nodiscard]] bool ready(const Handle &resource) const;
    };

    /// @brief Readiness check for a buffer published by the Buffer manager.
    struct BufferReadiness
    {
        core::device::manager::Buffer *manager{nullptr};

        /// @brief Block until the record's future publishes, then validate the buffer exists.
        [[nodiscard]] bool ready(const Handle &resource) const;
    };

    /// @brief Readiness check for a legacy ManagerRenderResource buffer (engine vertex/instance/uniform buffers).
    struct RenderResourceBufferReadiness
    {
        Handle deviceID{};

        /// @brief Block until the transfer worker reports completion.
        /// @details The worker sets the completion flag on success and failure, so this guarantees the transfer task
        /// ran; there is no error channel to validate.
        [[nodiscard]] bool ready(const Handle &resource) const;
    };

    /// @brief A host-side readiness wait for one transfer resource.
    struct CPURecordingWait
    {
        Handle resource;
        std::variant<ImageReadiness, BufferReadiness, RenderResourceBufferReadiness> readiness;
    };

    /// @brief Register waits that must pass before the pass records.
    /// @details Deduplicated by resource handle (last registration wins).
    void add(std::vector<CPURecordingWait> waits);

    /// @brief Run every registered wait, clear them, and return the resources whose check failed or threw.
    /// @details An empty result means every resource was ready. Exceptions are treated as a failed check so one bad
    /// producer cannot abort the pass submission.
    [[nodiscard]] std::vector<Handle> runAll();

    [[nodiscard]] bool empty() const noexcept
    {
        return m_waits.empty();
    }

    void clear() noexcept
    {
        m_waits.clear();
    }

  private:
    std::vector<CPURecordingWait> m_waits;
};
} // namespace star
