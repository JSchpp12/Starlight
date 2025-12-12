#pragma once

#include <starlight/common/IEvent.hpp>
#include <vulkan/vulkan.hpp>

#include <vector>

namespace star::event
{
constexpr std::string_view GetConsumeDescriptorRequestsTypeName = "star::event::ConsumeDescriptorRequests";

class ConsumeDescriptorRequests : public common::IEvent
{
  public:
    explicit ConsumeDescriptorRequests(void *requestsData);

    virtual ~ConsumeDescriptorRequests() = default;

    void *getData() const
    {
        return m_requestsData;
    }

  private:
    void *m_requestsData;
};

static inline std::vector<std::pair<vk::DescriptorType, const uint32_t>> *GetDestination(
    const ConsumeDescriptorRequests &e)
{
    return static_cast<std::vector<std::pair<vk::DescriptorType, const uint32_t>> *>(e.getData());
}
} // namespace star::event