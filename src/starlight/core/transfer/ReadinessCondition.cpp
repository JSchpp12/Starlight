#include "starlight/core/transfer/ReadinessCondition.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/Image.hpp"
#include "core/device/managers/Pipeline.hpp"
#include "core/device/managers/Shader.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "managers/ManagerRenderResource.hpp"

#include <thread>

namespace star
{
ReadinessState ImageReadiness::check(const Handle &resource) const
{
    if (manager == nullptr)
        return ReadinessState::Failed;

    if (manager->isReady(resource))
        return ReadinessState::Ready;
    return manager->isPending(resource) ? ReadinessState::Pending : ReadinessState::Failed;
}

ReadinessState BufferReadiness::check(const Handle &resource) const
{
    if (manager == nullptr)
        return ReadinessState::Failed;

    if (manager->isReady(resource))
        return ReadinessState::Ready;
    return manager->isPending(resource) ? ReadinessState::Pending : ReadinessState::Failed;
}

ReadinessState RenderResourceBufferReadiness::check(const Handle &resource) const
{
    if (!deviceID.isInitialized())
        return ReadinessState::Failed;

    if (ManagerRenderResource::isReady(deviceID, resource))
        return ReadinessState::Ready;
    return ManagerRenderResource::isPending(deviceID, resource) ? ReadinessState::Pending : ReadinessState::Failed;
}

ReadinessState ShaderReadiness::check(const Handle &resource) const
{
    if (manager == nullptr)
        return ReadinessState::Failed;

    if (manager->isReady(resource))
        return ReadinessState::Ready;
    return manager->isPending(resource) ? ReadinessState::Pending : ReadinessState::Failed;
}

ReadinessState PipelineReadiness::check(const Handle &resource) const
{
    if (manager == nullptr || !resource.isInitialized())
        return ReadinessState::Failed;

    // Cheap path: Manager::isReady resolves a ready record-level future. Avoid draining the complete-message queue
    // while already built.
    if (manager->isReady(resource))
        return ReadinessState::Ready;

    // The build is queued from the main-thread completion path, so pump completions before inspecting state.
    if (context != nullptr)
        context->manualTriggerOfCheckForMessages();

    const auto *record = manager->get(resource);
    if (record == nullptr)
        return ReadinessState::Failed;

    if (record->isReady())
        return ReadinessState::Ready;

    // A still-engaged future means the build has not published yet; no future and no pipeline means it failed.
    return record->pending.has_value() ? ReadinessState::Pending : ReadinessState::Failed;
}

ReadinessState ReadinessCondition::check() const
{
    return std::visit([this](const auto &readiness) { return readiness.check(resource); }, readiness);
}

ReadinessState EvaluateReadiness(const std::vector<ReadinessCondition> &conditions,
                                 std::chrono::steady_clock::time_point deadline, std::vector<ReadinessState> *outStates)
{
    if (outStates != nullptr)
        outStates->assign(conditions.size(), ReadinessState::Pending);

    if (conditions.empty())
        return ReadinessState::Ready;

    while (true)
    {
        bool pending = false;
        bool failed = false;
        for (size_t i = 0; i < conditions.size(); ++i)
        {
            ReadinessState state = ReadinessState::Failed;
            try
            {
                state = conditions[i].check();
            }
            catch (...)
            {
                // A stale/removed handle or a throwing producer must not abort pass submission.
                state = ReadinessState::Failed;
            }

            if (outStates != nullptr)
                (*outStates)[i] = state;

            if (state == ReadinessState::Failed)
                failed = true;
            else if (state == ReadinessState::Pending)
                pending = true;
        }

        // Evaluate every condition even after a failure so callers receive the complete failed set.
        if (failed)
            return ReadinessState::Failed;

        if (!pending)
            return ReadinessState::Ready;

        if (std::chrono::steady_clock::now() >= deadline)
            return ReadinessState::Pending;

        // sleep to wait in case ready state changes to ready
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool WaitForReadiness(const std::vector<ReadinessCondition> &conditions, std::chrono::milliseconds timeout)
{
    const ReadinessState state = EvaluateReadiness(conditions, std::chrono::steady_clock::now() + timeout);
    if (state == ReadinessState::Ready)
        return true;

    core::logging::warning("Readiness wait did not complete for ", conditions.size(),
                           " condition(s); state=", state == ReadinessState::Failed ? "failed" : "pending");
    return false;
}
} // namespace star
