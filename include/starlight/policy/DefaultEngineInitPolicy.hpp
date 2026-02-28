#pragma once

#include "starlight/service/Service.hpp"

#include <star_common/FrameTracker.hpp>
#include <vulkan/vulkan.hpp>

#include <set>

// creates non-internactive devices and vulkan rendering instance
namespace star::policy
{
class DefaultEngineInitPolicy
{
  public:
    virtual ~DefaultEngineInitPolicy() = default;

    void init(uint8_t requestedNumFramesInFlight);
    void cleanup(core::RenderingInstance &instance);
    core::device::StarDevice createNewDevice(core::RenderingInstance &renderingInstance,
                                             std::set<star::Rendering_Features> &engineRenderingFeatures,
                                             std::set<Rendering_Device_Features> &engineRenderingDeviceFeatures);

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

    static service::Service createResourceOwnershipService(); 

  protected:
    virtual std::vector<service::Service> addAdditionalServices()
    {
        return {};
    };

  private:
    uint8_t m_maxNumFramesInFlight{1};
};
} // namespace star::policy