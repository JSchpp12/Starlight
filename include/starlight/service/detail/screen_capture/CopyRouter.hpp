#pragma once

#include "CalleeRenderDependencies.hpp"
#include "CapabilityCache.hpp"
#include "CopyPlan.hpp"
#include "PerExtentResources.hpp"

namespace star::service::detail::screen_capture
{
class CopyRouter
{
  public:
    CopyRouter() = default;

    explicit CopyRouter(vk::PhysicalDevice pd) : m_deviceCapabilities(std::move(pd))
    {
    }

    void init(DeviceInfo *deviceInfo);

    CopyPlan decide(CalleeRenderDependencies &deps, const Handle &calleeRegistration,
                    const uint8_t &frameInFlightIndex);

    void cleanupRender(DeviceInfo *deviceInfo)
    {
        assert(deviceInfo != nullptr);

        m_resourceContainer.cleanupRender();
    }

  private:
    CapabilityCache m_deviceCapabilities;
    PerExtentResources m_resourceContainer;

    void decideRoute(CalleeRenderDependencies &deps, common::RoutePath &route, vk::Filter &copyFilter);

    CopyResource decideResourcesToUse(const CalleeRenderDependencies &deps, const Handle &calleeRegistration,
                                      const uint8_t &frameInFlightIndex);
};
} // namespace star::service::detail::screen_capture