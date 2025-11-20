#pragma once

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>
#include <starlight/common/IEvent.hpp>

#include <cassert>
#include <utility>

namespace star::core::device::system::event
{
template <typename TManagerRequest> class ManagerRequest : public star::common::IEvent
{
  public:
    ManagerRequest(uint16_t registeredEventType, TManagerRequest request, Handle &resultingHandle,
                   std::optional<void **> resultingResource = std::nullopt)
        : star::common::IEvent(std::move(registeredEventType)), m_request(std::move(request)),
          m_resultingHandle(resultingHandle), m_resultingResource(resultingResource)
    {
    }

    TManagerRequest giveMeRequest() const
    {
        return m_request;
    }
    Handle &getResultingHandle() const
    {
        return m_resultingHandle;
    }

    std::optional<void **> getResultingResourcePointer() const
    {
        return m_resultingResource;
    }

  private:
    TManagerRequest m_request;
    Handle &m_resultingHandle;
    std::optional<void **> m_resultingResource ;
};
} // namespace star::core::device::system::event