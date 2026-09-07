#pragma once

#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/IStartupDeviceRequirementsProvider.hpp"
#include "starlight/service/Service.hpp"

#include <star_common/FrameTracker.hpp>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>
#include <set>
#include <utility>

// creates non-internactive devices and vulkan rendering instance
namespace star::policy
{
class DefaultEngineInitPolicy
{
  public:
    using LoadAdditionalServices = std::function<std::vector<service::Service>()>;
    using StartupDeviceRequirementsProvider = core::device::IStartupDeviceRequirementsProvider;

    DefaultEngineInitPolicy() = default;
    explicit DefaultEngineInitPolicy(LoadAdditionalServices addServiceLoader)
        : m_addServiceLoader(std::move(addServiceLoader)) {};
    explicit DefaultEngineInitPolicy(std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements)
        : m_startupDeviceRequirements(std::move(startupDeviceRequirements)) {};
    DefaultEngineInitPolicy(LoadAdditionalServices addServiceLoader,
                            std::unique_ptr<StartupDeviceRequirementsProvider> startupDeviceRequirements)
        : m_addServiceLoader(std::move(addServiceLoader)),
          m_startupDeviceRequirements(std::move(startupDeviceRequirements)) {};

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

    static service::Service createScreenCaptureService();

    static service::Service createIOService();

    static service::Service createFrameInFlightControllerService();

    static service::Service createHeadlessCaptureService();

    static service::Service createSceneLoaderService();

    static service::Service createCommandOrderService();

    static service::Service createShaderService();

  protected:
    virtual std::vector<service::Service> addAdditionalServices()
    {
        return {};
    };

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
    LoadAdditionalServices m_addServiceLoader;
    std::unique_ptr<StartupDeviceRequirementsProvider> m_startupDeviceRequirements;
    bool m_startupRequirementsConsumed{false};
    uint8_t m_maxNumFramesInFlight{1};
};
} // namespace star::policy
