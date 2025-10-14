#pragma once

#include "Handle.hpp"
#include "StarCommandBuffer.hpp"
#include "core/DeviceContext.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace star::ManagerController
{
template <typename T> class Controller
{
  public:
    Controller() = default;

    Controller(bool shouldUpdateAfterCreation)
        : m_shouldUpdateAfterCreation(std::move(shouldUpdateAfterCreation)), m_resourceHandles(std::vector<Handle>(1))
    {
    }

    Controller(bool shouldUpdateAfterCreation, const uint8_t &numFramesInFlight)
        : m_shouldUpdateAfterCreation(std::move(shouldUpdateAfterCreation)),
          m_resourceHandles(std::vector<Handle>(numFramesInFlight))
    {
    }

    virtual ~Controller() = default;

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
    {
        assert(m_resourceHandles.size() == 1 |
                   (m_resourceHandles.size() != 1 && m_resourceHandles.size() == numFramesInFlight) &&
               "Invalid setup for data");
        for (uint8_t i = 0; i < m_resourceHandles.size(); i++)
        {
            // create request
        }
    }

    void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
    {
        assert(m_shouldUpdateAfterCreation && frameInFlightIndex <= m_resourceHandles.size() &&
               "Requested frame in flight index is beyond storage of updating Controller");

        if (m_shouldUpdateAfterCreation && isValid(frameInFlightIndex))
        {
            // create new request or update to manager
            //  context.getManagerRenderResource().
        }
    }

    const Handle &getHandle(const uint8_t &index) const
    {
        assert(index <= m_resourceHandles.size() && "Ensure index is within range of requested resource size");

        return m_resourceHandles[index];
    }

  protected:
    virtual bool isValid(const uint8_t &currentFrameInFlightIndex) const
    {
        if (m_shouldUpdateAfterCreation && currentFrameInFlightIndex == m_shouldUpdateAfterCreation)
            return false;
        return true;
    };

    virtual std::unique_ptr<T> createTransferRequest(core::device::StarDevice &device,
                                                     const uint8_t &frameInFlightIndex) = 0;

  private:
    bool m_shouldUpdateAfterCreation = false;
    std::vector<Handle> m_resourceHandles = std::vector<Handle>();
};
} // namespace star::ManagerController