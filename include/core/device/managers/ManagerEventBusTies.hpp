#pragma once

#include "Manager.hpp"

#include "Enums.hpp"
#include "core/device/system/EventBus.hpp"
#include "core/device/system/event/ManagerRequest.hpp"

namespace star::core::device::manager
{
template <typename TRecord, typename TResourceRequest, star::Handle_Type THandleType, size_t TMaxRecordCount>
class ManagerEventBusTies : public Manager<TRecord, TResourceRequest, THandleType, TMaxRecordCount>
{
  public:
    ManagerEventBusTies() = default;
    virtual ~ManagerEventBusTies() = default;

    ManagerEventBusTies(const ManagerEventBusTies &) = delete;
    ManagerEventBusTies &operator=(const ManagerEventBusTies &) = delete;
    ManagerEventBusTies(ManagerEventBusTies &&) = delete;
    ManagerEventBusTies &operator=(ManagerEventBusTies &&) = delete;

    void init(std::shared_ptr<device::StarDevice> device, core::device::system::EventBus &bus)
    {
        m_device = device;

        submitSubscribeToEventBus(bus);
    }

    virtual void cleanup(core::device::system::EventBus &bus)
    {
        assert(m_subscriberInfo.isInitialized() && "Should not call cleanup twice");
        bus.unsubscribe<core::device::system::event::ManagerRequest<TResourceRequest>>(m_subscriberInfo);

        m_subscriberInfo = Handle();
    }

  protected:
    std::shared_ptr<device::StarDevice> m_device;
    Handle m_subscriberInfo;

    void submitSubscribeToEventBus(core::device::system::EventBus &bus)
    {
        bus.subscribe<core::device::system::event::ManagerRequest<TResourceRequest>>(
            [this](const core::device::system::EventBase &e, bool &keepAlive) {
                const auto &requestEvent =
                    static_cast<const core::device::system::event::ManagerRequest<TResourceRequest> &>(e);
                requestEvent.getResultingHandle() = this->submit(*this->m_device, requestEvent.giveMeRequest());
                keepAlive = true;
            },
            [this]() -> Handle * { return &this->m_subscriberInfo; },
            [this](const Handle &noLongerNeededSubscriberHandle) {
                if (this->m_subscriberInfo == noLongerNeededSubscriberHandle)
                {
                    this->m_subscriberInfo = Handle();
                }
            });
    };
};
} // namespace star::core::device::manager