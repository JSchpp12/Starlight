#include "core/device/managers/DescriptorPool.hpp"

#include "event/ConsumeDescriptorRequests.hpp"
#include "event/EnginePhaseComplete.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <cassert>

namespace star::core::device::manager
{
DescriptorPool::DescriptorPool() : Manager<DescriptorPoolRecord, DescriptorPoolRequest, 1>(GetDescriptorPoolTypeName)
{
}

void DescriptorPool::init(const uint8_t &numFramesInFlight, device::StarDevice *device, common::EventBus &eventBus)
{
    init(device);

    m_numFramesInFlight = numFramesInFlight;
    m_eventBus = &eventBus;
    registerListenForEnginePhaseComplete(eventBus);
}

void DescriptorPool::init(device::StarDevice *device)
{
    Manager<DescriptorPoolRecord, DescriptorPoolRequest, 1>::init(device);
}

void DescriptorPool::registerListenForEnginePhaseComplete(common::EventBus &bus)
{
    uint16_t type =
        common::HandleTypeRegistry::instance().registerType(star::event::GetEnginePhaseCompleteInitTypeName());
    bus.subscribe(type,
                  common::SubscriberCallbackInfo(
                      std::bind(&DescriptorPool::eventCallback, this, std::placeholders::_1, std::placeholders::_2),
                      std::bind(&DescriptorPool::getHandleForUpdate, this),
                      std::bind(&DescriptorPool::notificationFromEventBusHandleDelete, this, std::placeholders::_1)));
}

void DescriptorPool::eventCallback(const star::common::IEvent &e, bool &keepAlive)
{
    // on engine phase complete we will now need to emit another event
    auto requests = emitAndGetRequests();

    absl::flat_hash_map<vk::DescriptorType, uint32_t> processedTypes;

    for (size_t i = 0; i < requests.size(); i++)
    {
        if (processedTypes.contains(requests[i].first))
        {
            processedTypes.find(requests[i].first)->second += requests[i].second;
        }
        else
        {
            processedTypes.insert(std::make_pair(requests[i].first, requests[i].second));
        }
    }

    this->insert({std::move(processedTypes)});

    keepAlive = false;
}

DescriptorPoolRecord DescriptorPool::createRecord(DescriptorPoolRequest &&request) const
{
    uint32_t maxSets = 0;

    auto builder = StarDescriptorPool::Builder(*this->m_device);
    for (auto &active : request.requests)
    {
        builder.addPoolSize(active.first, active.second);
        maxSets += active.second;
    }

    builder.setMaxSets(maxSets);

    return {builder.build()};
}

void DescriptorPool::notificationFromEventBusHandleDelete(const Handle &noLongerNeededSubscriberHandle)
{
    if (m_engineSceneInitCallbackDone == noLongerNeededSubscriberHandle)
    {
        m_engineSceneInitCallbackDone = Handle();
    }
}

std::vector<std::pair<vk::DescriptorType, const uint32_t>> DescriptorPool::emitAndGetRequests()
{
    assert(this->m_eventBus != nullptr);

    std::vector<std::pair<vk::DescriptorType, const uint32_t>> data;

    this->m_eventBus->emit(event::ConsumeDescriptorRequests{static_cast<void *>(&data)});

    return data;
}

} // namespace star::core::device::manager