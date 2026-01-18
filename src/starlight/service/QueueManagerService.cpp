#include "starlight/service/QueueManagerService.hpp"

#include "starlight/core/Exceptions.hpp"
#include <star_common/special_types/SpecialHandleTypes.hpp>

namespace star::service
{
QueueManagerService::QueueManagerService(std::vector<Handle> allQueueHandles,
                                         absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues)
    : m_records(star::common::special_types::QueueTypeName), m_engineReservedQueues(std::move(engineReservedQueues)),
      m_getQueueListener(*this), m_eventBus(nullptr)
{
    for (const auto &handle : allQueueHandles)
    {
        m_records.manualInsert(handle, QueueOwnershipInfo{true});
    }
}

QueueManagerService::QueueManagerService(QueueManagerService &&other)
    : m_records(std::move(other.m_records)), m_engineReservedQueues(std::move(other.m_engineReservedQueues)),
      m_getQueueListener(*this), m_eventBus(other.m_eventBus), m_queueManager(other.m_queueManager)
{
    if (m_eventBus != nullptr)
    {
        other.cleanup(*m_eventBus);
        initListeners(*m_eventBus);
    }
}

QueueManagerService &QueueManagerService::operator=(QueueManagerService &&other)
{
    // TODO: insert return statement here
    if (this != &other)
    {
        m_records = std::move(other.m_records);
        m_engineReservedQueues = (std::move(other.m_engineReservedQueues));

        if (m_eventBus != nullptr)
        {
            other.cleanup(*m_eventBus);
            initListeners(*m_eventBus);
        }
    }

    return *this;
}

void QueueManagerService::cleanup(common::EventBus &eventBus)
{
    cleanupListeners(eventBus);
}

void QueueManagerService::setInitParameters(star::service::InitParameters &params)
{
    m_eventBus = &params.eventBus;
    m_queueManager = &params.graphicsManagers.queueManager;
}

void QueueManagerService::init(const uint8_t &numFramesInFlight)
{
    assert(m_eventBus != nullptr);
    initListeners(*m_eventBus);
}

void QueueManagerService::shutdown()
{
    assert(m_eventBus != nullptr);
    cleanup(*m_eventBus);
}

void QueueManagerService::onGetQueue(const event::GetQueue &event, bool &keepAlive)
{
    // find the queue with the requested caps
    Handle selected =
        searchForQueue(event.getRequestedQueueType(), event.getQueueFamilyInfo(), event.getRequestEngineReservedQueue(),
                       event.getAvoidIndex(), event.getSelectFromIndex());

    if (selected.isInitialized())
    {
        m_records.get(selected).isAvailable = false;
    }
    *event.getQueueResultLocation() = std::move(selected);
    keepAlive = true;
}

void QueueManagerService::initListeners(common::EventBus &eventBus)
{
    m_getQueueListener.init(eventBus);
}

void QueueManagerService::cleanupListeners(common::EventBus &eventBus)
{
    m_getQueueListener.cleanup(eventBus);
}

Handle QueueManagerService::getAvailableQueueWithCaps(const vk::QueueFlags &caps, const uint8_t *familyIndexToAvoid)
{
    Handle selectedHandle;

    for (const auto &record : m_records.getRecords())
    {
        if (record.second.isAvailable && m_queueManager->get(record.first)->queue.isCompatibleWith(caps))
        {
            if (familyIndexToAvoid == nullptr ||
                (familyIndexToAvoid != nullptr &&
                 m_queueManager->get(record.first)->queue.getParentQueueFamilyIndex() != *familyIndexToAvoid))
            {
                selectedHandle = record.first;
                break;
            }
        }
    }

    if (selectedHandle.isInitialized())
    {
        m_records.get(selectedHandle).isAvailable = false;
    }

    return selectedHandle;
}

Handle QueueManagerService::getAvailableQueueOfTypeAvoidIndex(const star::Queue_Type &type,
                                                              const uint8_t *familyIndexToAvoid)
{
    // convert queue_type to vk::QueueFlags
    if (type == star::Queue_Type::Tpresent)
    {
        return m_engineReservedQueues.at(star::Queue_Type::Tpresent);
    }

    vk::QueueFlags flags;

    switch (type)
    {
    case (star::Queue_Type::Tgraphics):
        flags = vk::QueueFlagBits::eGraphics;
        break;
    case (star::Queue_Type::Tcompute):
        flags = vk::QueueFlagBits::eCompute;
        break;
    case (star::Queue_Type::Ttransfer):
        flags = vk::QueueFlagBits::eTransfer;
        break;
    default:
        STAR_THROW("Unknown queue type encountered");
    }

    return getAvailableQueueWithCaps(flags, familyIndexToAvoid);
}

Handle QueueManagerService::getAvailableQueueOfTypeFromIndex(const star::Queue_Type &type,
                                                             const uint8_t &selectFromFamilyIndex)
{
    vk::QueueFlags flags;

    switch (type)
    {
    case (star::Queue_Type::Tgraphics):
        flags = vk::QueueFlagBits::eGraphics;
        break;
    case (star::Queue_Type::Tcompute):
        flags = vk::QueueFlagBits::eCompute;
        break;
    case (star::Queue_Type::Ttransfer):
        flags = vk::QueueFlagBits::eTransfer;
        break;
    default:
        STAR_THROW("Unknown queue type encountered");
    }

    Handle selected;
    for (auto &record : m_records.getRecords())
    {
        const auto &queue = m_queueManager->get(record.first)->queue;

        if (queue.getParentQueueFamilyIndex() == selectFromFamilyIndex && queue.isCompatibleWith(flags) &&
            record.second.isAvailable)
        {
            selected = record.first;
            break;
        }
    }

    return selected;
}

Handle QueueManagerService::getDefaultEngineQueue(const star::Queue_Type &caps)
{
    if (!m_engineReservedQueues.contains(caps))
    {
        STAR_THROW("No engine reserved queue of the provided type assigned at service creation");
    }

    return m_engineReservedQueues[caps];
}

Handle QueueManagerService::searchForQueue(const star::Queue_Type &type, const uint8_t *familyIndexData,
                                           bool requestDefaultEngineQueue, bool avoidFamilyIndex,
                                           bool selectFromFamilyIndex)
{
    if (requestDefaultEngineQueue)
    {
        return getDefaultEngineQueue(type);
    }
    else if (avoidFamilyIndex)
    {
        return getAvailableQueueOfTypeAvoidIndex(type, familyIndexData);
    }
    else if (selectFromFamilyIndex)
    {
        return getAvailableQueueOfTypeFromIndex(type, *familyIndexData);
    }

    return Handle();
}
} // namespace star::service