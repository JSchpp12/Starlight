#include "internals/CPURecordingWaitContainer.hpp"

#include "core/device/managers/Buffer.hpp"
#include "core/device/managers/Image.hpp"
#include "managers/ManagerRenderResource.hpp"

#include <algorithm>
#include <utility>

bool star::CPURecordingWaitContainer::ImageReadiness::ready(const Handle &resource) const
{
    if (manager == nullptr)
        return false;

    manager->waitForReady(resource);
    const auto *record = manager->get(resource);
    const auto *texture = record != nullptr ? record->getTexture() : nullptr;
    return texture != nullptr && texture->getVulkanImage() != VK_NULL_HANDLE;
}

bool star::CPURecordingWaitContainer::BufferReadiness::ready(const Handle &resource) const
{
    if (manager == nullptr)
        return false;

    manager->waitForReady(resource);
    const auto *record = manager->get(resource);
    const auto *buffer = record != nullptr ? record->getBuffer() : nullptr;
    return buffer != nullptr && buffer->getVulkanBuffer() != VK_NULL_HANDLE;
}

bool star::CPURecordingWaitContainer::RenderResourceBufferReadiness::ready(const Handle &resource) const
{
    ManagerRenderResource::waitForReady(deviceID, resource);
    return true;
}

void star::CPURecordingWaitContainer::add(std::vector<CPURecordingWait> waits)
{
    for (auto &wait : waits)
    {
        // Deduplicate by resource: the latest wait for a handle wins. The list is small (one entry per resource the
        // pass consumes before recording), so a linear scan is cheaper than maintaining a map alongside it.
        const auto existing =
            std::find_if(m_waits.begin(), m_waits.end(), [&wait](const CPURecordingWait &registered) {
                return registered.resource == wait.resource;
            });

        if (existing != m_waits.end())
            existing->readiness = std::move(wait.readiness);
        else
            m_waits.push_back(std::move(wait));
    }
}

std::vector<star::Handle> star::CPURecordingWaitContainer::runAll()
{
    std::vector<Handle> failed;

    for (auto &wait : m_waits)
    {
        bool ready = true;
        try
        {
            ready = std::visit(
                [&resource = wait.resource](const auto &readiness) { return readiness.ready(resource); },
                wait.readiness);
        }
        catch (...)
        {
            ready = false;
        }

        if (!ready)
            failed.push_back(wait.resource);
    }

    m_waits.clear();
    return failed;
}
