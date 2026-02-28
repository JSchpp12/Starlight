#include "TaskManager.hpp"

#include <star_common/helper/CastHelpers.hpp>

#include <cassert>

namespace star::job
{

Handle TaskManager::registerWorker(worker::Worker newWorker, std::string_view taskName) noexcept
{
    Handle newRegistration = registerNewTaskType(taskName);

    registerWorker(std::move(newWorker), newRegistration);
    return newRegistration;
}

Handle TaskManager::registerNewTaskType(std::string_view taskName) noexcept
{
    std::optional<uint16_t> type = common::HandleTypeRegistry::instance().getType(taskName);

    if (!type.has_value())
    {
        type = std::make_optional(common::HandleTypeRegistry::instance().registerType(taskName));
    }

    return Handle{.type = type.value(), .id = 0};
}

void TaskManager::registerWorker(worker::Worker newWorker, Handle &registeredTaskTypeHandle) noexcept
{
    newWorker.setCompleteMessageCommunicationStructure(m_completeTasks.get());

    m_workers[registeredTaskTypeHandle.getType()].emplace_back(std::move(newWorker));

    size_t index = m_workers[registeredTaskTypeHandle.getType()].size();
    assert(index >= 1);
    index--;

    common::helper::SafeCast<size_t, uint32_t>(index,
                                            registeredTaskTypeHandle.id);
}

void TaskManager::cleanup() noexcept
{
    for (auto &[type, list] : m_workers)
    {
        for (auto &w : list)
        {
            w.cleanup();
        }
    }
}

size_t TaskManager::getNumOfWorkersForType(const Handle &registeredTaskType) const noexcept
{
    auto *pool = getWorkerPool(registeredTaskType.getType());

    if (pool == nullptr)
    {
        return 0;
    }

    return pool->size();
}

bool TaskManager::isThereWorkerForTask(const Handle &taskType) noexcept
{
    const worker::Worker *found = getWorker(taskType);

    return found != nullptr;
}

worker::Worker *TaskManager::getWorker(const Handle &registeredTaskType) noexcept
{
    auto type = registeredTaskType.getType();

    auto *pool = getWorkerPool(registeredTaskType.getType());

    if (pool == nullptr)
    {
        return nullptr;
    }

    size_t workerIndex = 0;
    common::helper::SafeCast<uint16_t, size_t>(registeredTaskType.getID(), workerIndex);

    return &pool->at(workerIndex);
}
std::vector<worker::Worker> *TaskManager::getWorkerPool(const uint16_t &registeredType) noexcept
{
    auto it = m_workers.find(registeredType);
    if (it == m_workers.end())
    {
        return nullptr;
    }

    return &it->second;
}
const std::vector<worker::Worker> *TaskManager::getWorkerPool(const uint16_t &registeredType) const noexcept
{
    auto it = m_workers.find(registeredType);
    if (it == m_workers.end())
    {
        return nullptr;
    }

    return &it->second;
}
} // namespace star::job