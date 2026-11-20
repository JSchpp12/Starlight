#include "internals/OneTimeCommandBufferSynchronizer.hpp"

#include "core/device/managers/Semaphore.hpp"

#include <cassert>
#include <utility>

namespace star
{
void OneTimeCommandBufferSynchronizer::prepRender(core::device::manager::Semaphore &semaphoreManager)
{
    m_output.clear();
    m_output.reserve(m_entries.size());

    for (const auto &entry : m_entries)
    {
        const auto *record = semaphoreManager.get(entry.record);
        assert(record != nullptr && "one-time wait record is not registered with the semaphore manager");

        m_output.push_back(StarCommandBuffer::Data{
            .semaphore = record->semaphore, .waitPoints = entry.stages, .signaledValue = record->timelineValue});
    }

    m_prepared = true;
}

void OneTimeCommandBufferSynchronizer::registerWait(const Handle &semaphoreRecord, vk::PipelineStageFlags stages)
{
    assert(semaphoreRecord.isInitialized() && "one-time GPU waits must be keyed by a semaphore-manager record Handle");

    const auto existing = m_entries.find(WaitEntry{semaphoreRecord, vk::PipelineStageFlags{0}});
    if (existing != m_entries.end())
    {
        const auto mergedStages = existing->stages | stages;
        m_entries.erase(existing);
        m_entries.insert(WaitEntry{semaphoreRecord, mergedStages});
        return;
    }

    m_entries.insert(WaitEntry{semaphoreRecord, stages});
}

void OneTimeCommandBufferSynchronizer::manual_add_data(StarCommandBuffer::Data data)
{
    m_manual.push_back(std::move(data));
}

std::span<const StarCommandBuffer::Data> OneTimeCommandBufferSynchronizer::drain()
{
    assert(m_prepared && "prepRender must run before drain");

    for (auto &data : m_manual)
        m_output.push_back(std::move(data));

    m_manual.clear();
    m_entries.clear();
    m_prepared = false;

    return std::span<const StarCommandBuffer::Data>{m_output.data(), m_output.size()};
}

void OneTimeCommandBufferSynchronizer::clearOutput() noexcept
{
    m_output.clear();
}
} // namespace star
