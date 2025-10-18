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

    virtual ~Controller() = default;

    virtual bool needsUpdated(const uint8_t &currentFrameInFlightIndex) const = 0;

    void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
    {
        assert(m_resourceHandles.size() == 0 && "Should not be initialized more than once");

        m_resourceHandles.resize(numFramesInFlight);

        for (uint8_t i = 0; i < numFramesInFlight; i++)
        {
            const auto semaphore = context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest{false});
            m_resourceHandles[i] = context.getManagerRenderResource().addRequest(
                context.getDeviceID(), context.getSemaphoreManager().get(semaphore)->semaphore, true);
        }
    }

    Handle getHandle(const uint8_t &index) const
    {
        assert(index <= m_resourceHandles.size() && "Ensure index is within range of requested resource size");

        return m_resourceHandles[index];
    }

    /// Call any frame updates. Returns true if the controller submitted an update
    virtual bool submitUpdateIfNeeded(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                                      vk::Semaphore &semaphore) = 0;

  protected:
    std::vector<Handle> m_resourceHandles = std::vector<Handle>();

    virtual std::unique_ptr<T> createTransferRequest(core::device::StarDevice &device,
                                                     const uint8_t &frameInFlightIndex) = 0;
};
} // namespace star::ManagerController