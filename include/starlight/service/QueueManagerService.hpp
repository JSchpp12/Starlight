#pragma once

#include "starlight/core/MappedHandleContainer.hpp"
#include "starlight/core/device/managers/Queue.hpp"
#include "starlight/enums/Enums.hpp"
#include "starlight/policy/ListenForGetQueuePolicy.hpp"
#include "starlight/service/InitParameters.hpp"

#include <absl/container/flat_hash_map.h>

namespace star::service
{
class QueueManagerService
{
  public:
    QueueManagerService(std::vector<Handle> allQueueHandles,
                        absl::flat_hash_map<star::Queue_Type, Handle> engineReservedQueues);
    QueueManagerService(const QueueManagerService &) = delete;
    QueueManagerService &operator=(const QueueManagerService &) = delete;
    QueueManagerService(QueueManagerService &&other);
    QueueManagerService &operator=(QueueManagerService &&other); 
    ~QueueManagerService() = default;
    void init();

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void cleanup(common::EventBus &eventBus);

    void onGetQueue(const event::GetQueue &event, bool &keepAlive);

  private:
    struct QueueOwnershipInfo
    {
        bool isAvailable = true;
    };

    core::MappedHandleContainer<QueueOwnershipInfo> m_records;
    absl::flat_hash_map<star::Queue_Type, Handle> m_engineReservedQueues;
    policy::ListenForGetQueuePolicy<QueueManagerService> m_getQueueListener;
    common::EventBus *m_eventBus = nullptr;
    core::device::manager::Queue *m_queueManager = nullptr;

    void initListeners(common::EventBus &eventBus);

    void cleanupListeners(common::EventBus &eventBus); 

    Handle getAvailableQueueWithCaps(const vk::QueueFlags &caps, const uint8_t *familyIndexToAvoid);
    Handle getAvailableQueueOfTypeAvoidIndex(const star::Queue_Type &type, const uint8_t *familyIndexToAvoid);
    Handle getAvailableQueueOfTypeFromIndex(const star::Queue_Type &type, const uint8_t &selectFromFamilyIndex); 
    Handle getDefaultEngineQueue(const star::Queue_Type &caps);

    Handle searchForQueue(const star::Queue_Type &caps, const uint8_t *familyIndexInfo, bool requestDefaultEngineQueue,
                          bool avoidFamilyIndex, bool selectFromFamilyInfo);
};
} // namespace star::service