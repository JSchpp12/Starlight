#pragma once

#include "StarDescriptorBuilders.hpp"
#include "core/device/managers/Manager.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <stack>
#include <unordered_map>

namespace star::core::device::manager
{
struct DescriptorPoolRequest
{
    absl::flat_hash_map<vk::DescriptorType, uint32_t> requests;
};

struct DescriptorPoolRecord
{
    std::unique_ptr<StarDescriptorPool> pool;
    bool isReady() const
    {
        return pool != nullptr;
    }

    void cleanupRender(star::core::device::StarDevice &context)
    {
        pool.reset();
    }
};
constexpr std::string_view GetDescriptorPoolTypeName = "star::DescriptorPool";

class DescriptorPool : public Manager<DescriptorPoolRecord, DescriptorPoolRequest, 1>
{
  public:
    DescriptorPool();

    virtual ~DescriptorPool() = default;

    void init(const uint8_t &numFramesInFlight, device::StarDevice *device, common::EventBus &bus);

  private:
    std::unique_ptr<StarDescriptorPool> currentPool;
    Handle m_engineSceneInitCallbackDone;
    uint8_t m_numFramesInFlight = 0;
    common::EventBus *m_eventBus = nullptr;

    virtual void init(device::StarDevice *device) override;

    DescriptorPoolRecord createRecord(DescriptorPoolRequest &&request) const override;

    void eventCallback(const star::common::IEvent &e, bool &keepAlive);

    void registerListenForEnginePhaseComplete(common::EventBus &bus);

    Handle *getHandleForUpdate()
    {
        return &m_engineSceneInitCallbackDone;
    }

    void notificationFromEventBusHandleDelete(const Handle &noLongerNeededSubscriberHandle);

    std::vector<std::pair<vk::DescriptorType, const uint32_t>> emitAndGetRequests();

    static std::unordered_map<vk::DescriptorType, int> actives;
};
} // namespace star::core::device::manager