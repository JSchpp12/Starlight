#pragma once

#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/IStartupDeviceRequirementsProvider.hpp"
#include "starlight/service/EngineServices.hpp"

#include <star_common/FrameTracker.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <set>
#include <utility>

// creates non-internactive devices and vulkan rendering instance
namespace star::policy
{
class DefaultEngineInitPolicy
{
  public:
    using StartupDeviceRequirementsProvider = core::device::IStartupDeviceRequirementsProvider;

    DefaultEngineInitPolicy(service::EngineServices &engineServices,
                            std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements = {},
                            std::optional<int> overrideRenderingDeviceIndex = std::nullopt)
        : m_engineServices(engineServices), m_startupDeviceRequirements(std::move(startupDeviceRequirements)),
          m_overrideRenderingDeviceIndex(overrideRenderingDeviceIndex)
    {
    }

    DefaultEngineInitPolicy(const DefaultEngineInitPolicy &) = delete;
    DefaultEngineInitPolicy &operator=(const DefaultEngineInitPolicy &) = delete;
    DefaultEngineInitPolicy(DefaultEngineInitPolicy &&) = default;
    DefaultEngineInitPolicy &operator=(DefaultEngineInitPolicy &&) = default;

    virtual ~DefaultEngineInitPolicy() = default;

    void init(uint8_t requestedNumFramesInFlight);
    void cleanup(core::RenderingInstance &instance);
    virtual core::device::StarDevice createNewDevice(
        core::RenderingInstance &renderingInstance, std::set<Rendering_Device_Features> &engineRenderingDeviceFeatures);

    vk::Extent2D getEngineRenderingResolution();

    common::FrameTracker::Setup getFrameInFlightTrackingSetup(core::device::StarDevice &device);

    std::vector<service::Service> getAdditionalDeviceServices();

    core::RenderingInstance createRenderingInstance(std::string appName);

  protected:
    /// Consume the startup requirements provider. The provider is released as part of this call.
    core::device::DeviceRequirements consumeStartupDeviceRequirements()
    {
        if (m_startupRequirementsConsumed)
        {
            STAR_THROW("Startup device requirements have already been consumed");
        }

        m_startupRequirementsConsumed = true;
        auto provider = std::move(m_startupDeviceRequirements);
        return provider ? provider->getRequirements() : core::device::DeviceRequirements{};
    }

  private:
    service::EngineServices &m_engineServices;
    std::unique_ptr<StartupDeviceRequirementsProvider> m_startupDeviceRequirements;
    std::optional<int> m_overrideRenderingDeviceIndex{std::nullopt};
    bool m_startupRequirementsConsumed{false};
    uint8_t m_maxNumFramesInFlight{1};
};
} // namespace star::policy
