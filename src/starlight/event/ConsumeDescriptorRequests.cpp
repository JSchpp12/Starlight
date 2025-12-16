#include "event/ConsumeDescriptorRequests.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
ConsumeDescriptorRequests::ConsumeDescriptorRequests(void *requestsData)
    : common::IEvent(
          common::HandleTypeRegistry::instance().getTypeGuaranteedExist(GetConsumeDescriptorRequestsTypeName)),
      m_requestsData(requestsData)
{
}
} // namespace star::event