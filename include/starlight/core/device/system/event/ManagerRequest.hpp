#pragma once

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <star_common/IEvent.hpp>

#include <cassert>
#include <type_traits>
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
        // Requests are consumed exactly once by the manager's event callback.
        // Move out when the request owns move-only state (e.g. a compiled
        // StarShader); copyable request types keep their existing behaviour.
        if constexpr (std::is_copy_constructible_v<TManagerRequest>)
        {
            return m_request;
        }
        else
        {
            return std::move(const_cast<ManagerRequest *>(this)->m_request);
        }
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