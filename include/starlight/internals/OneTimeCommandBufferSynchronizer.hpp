#pragma once

#include "StarCommandBuffer.hpp"

#include <boost/container/flat_set.hpp>
#include <boost/container/small_vector.hpp>
#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <span>

namespace star::core::device::manager
{
class Semaphore;
}

namespace star
{
/// @brief One-shot registry of GPU timeline waits a command buffer must include in its next submission.
class OneTimeCommandBufferSynchronizer
{
  public:
    static constexpr std::size_t kInlineCapacity = 16;

    /// @brief Resolve every registered record into the persistent output buffer. Called by ManagerCommandBuffer.
    void prepRender(core::device::manager::Semaphore &semaphoreManager);

    /// @brief Register (or merge into) the wait for a semaphore record. Duplicate registrations OR their stages.
    void registerWait(const Handle &semaphoreRecord, vk::PipelineStageFlags stages);

    /// @brief Append an already-built wait to the internal scratch, taking ownership of it.
    void manual_add_data(StarCommandBuffer::Data data);

    /// @brief Append the manual entries to the prepared waits, clear the pending registrations, and return them.
    [[nodiscard]] std::span<const StarCommandBuffer::Data> drain();

    /// @brief Reset the output size to zero while retaining its capacity for reuse.
    void clearOutput() noexcept;

    void clear() noexcept
    {
        m_entries.clear();
        m_manual.clear();
        m_output.clear();
        m_prepared = false;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_entries.empty() && m_manual.empty();
    }

  private:
    struct WaitEntry
    {
        Handle record;
        vk::PipelineStageFlags stages{0};
    };

    struct WaitEntryCompare
    {
        bool operator()(const WaitEntry &lhs, const WaitEntry &rhs) const noexcept
        {
            if (lhs.record.getType() != rhs.record.getType())
                return lhs.record.getType() < rhs.record.getType();
            return lhs.record.getID() < rhs.record.getID();
        }
    };

    using WaitEntryStorage = boost::container::small_vector<WaitEntry, kInlineCapacity>;
    using WaitEntrySet = boost::container::flat_set<WaitEntry, WaitEntryCompare, WaitEntryStorage>;

    WaitEntrySet m_entries;
    boost::container::small_vector<StarCommandBuffer::Data, kInlineCapacity> m_manual;
    boost::container::small_vector<StarCommandBuffer::Data, kInlineCapacity> m_output;
    bool m_prepared{false};
};
} // namespace star
