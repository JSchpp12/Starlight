#pragma once

#include "core/device/system/event/EventBase.hpp"
#include "Handle.hpp"

#include <utility>

namespace star::core::device::system::event
{
template <typename TManagerRequest>
class ManagerRequest : public EventBase
{
  public:
    ManagerRequest(Handle &resultingHandle, TManagerRequest request)
        : m_resultingHandle(resultingHandle), m_request(std::move(request)) {};

    TManagerRequest giveMeRequest() const{
        return m_request; 
    }
    Handle &getResultingHandle() const{
      return m_resultingHandle; 
    }
  private:
    Handle &m_resultingHandle;
    TManagerRequest m_request;
};
} // namespace star::core::device::system::event