#include "internals/CPURecordingWaitContainer.hpp"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

namespace star
{

void CPURecordingWaitContainer::add(std::vector<CPURecordingWait> waits, ReadinessFailurePolicy policy)
{
    if (waits.empty())
        return;

    m_hasUnrun = true;

    for (auto &wait : waits)
    {
        insert(std::move(wait), std::move(policy));
    }
}

void CPURecordingWaitContainer::add(CPURecordingWait wait, ReadinessFailurePolicy policy)
{
    m_hasUnrun = true;

    insert(std::move(wait), std::move(policy));
}

CPURecordingWaitContainer::Result CPURecordingWaitContainer::runAll()
{
    m_hasUnrun = false;

    Result result;
    if (m_waits.empty())
        return result;

    result.hadWaits = true;

    std::vector<ReadinessCondition> conditions;
    conditions.reserve(m_waits.size());
    for (const auto &wait : m_waits)
        conditions.push_back(wait.condition);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    std::vector<ReadinessState> states;
    (void)EvaluateReadiness(conditions, deadline, &states);

    std::vector<RegisteredWait> retained;
    retained.reserve(m_waits.size());

    for (size_t i = 0; i < m_waits.size(); i++)
    {
        if (states[i] == ReadinessState::Failed)
        {
            result.failed.push_back(m_waits[i].condition.resource);
            if (ReadinessFailurePolicyPriority(m_waits[i].policy) > ReadinessFailurePolicyPriority(result.policy))
                result.policy = m_waits[i].policy;
        }
        else if (states[i] == ReadinessState::Pending)
        {
            result.pending.push_back(m_waits[i].condition.resource);
            retained.push_back(std::move(m_waits[i]));
        }
    }

    m_waits = std::move(retained);
    return result;
}

void CPURecordingWaitContainer::insert(CPURecordingWait &&newWait, ReadinessFailurePolicy policy)
{
    // Deduplicate by resource: the latest registration wins, including its policy.
    const auto existing = std::find_if(m_waits.begin(), m_waits.end(), [&newWait](const RegisteredWait &registered) {
        return registered.condition.resource == newWait.resource;
    });

    if (existing != m_waits.end())
    {
        existing->condition = std::move(newWait);
        existing->policy = policy;
    }
    else
    {
        m_waits.push_back({std::move(newWait), policy});
    }
}
} // namespace star
