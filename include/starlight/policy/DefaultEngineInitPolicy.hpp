#pragma once

#include "starlight/core/device/StarDevice.hpp"
#include "starlight/service/Service.hpp"

#include <vulkan/vulkan.hpp>
#include <star_common/FrameTracker.hpp>

#include <set>

// creates non-internactive devices and vulkan rendering instance
namespace star::policy
{
class DefaultEngineInitPolicy
{
  public:
    void init(uint8_t requestedNumFramesInFlight);
    void cleanup(core::RenderingInstance &instance);
    core::device::StarDevice createNewDevice(core::RenderingInstance &renderingInstance,
                                             std::set<star::Rendering_Features> &engineRenderingFeatures,
                                             std::set<Rendering_Device_Features> &engineRenderingDeviceFeatures);

    vk::Extent2D getEngineRenderingResolution();

    common::FrameTracker::Setup getFrameInFlightTrackingSetup(core::device::StarDevice &device);

    std::vector<service::Service> getAdditionalDeviceServices();

    core::RenderingInstance createRenderingInstance(std::string appName); 
  
  private:
    uint8_t m_maxNumFramesInFlight{1};

    service::Service createFrameInFlightControllerService() const; 
};
} // namespace star::policy