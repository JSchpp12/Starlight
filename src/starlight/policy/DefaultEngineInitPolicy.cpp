#include "starlight/policy/DefaultEngineInitPolicy.hpp"

namespace star::policy
{
void DefaultEngineInitPolicy::init(uint8_t requestedNumFramesInFLight)
{
}

void DefaultEngineInitPolicy::cleanup(core::RenderingInstance &instance)
{
}

star::core::device::StarDevice star::policy::DefaultEngineInitPolicy::createNewDevice(
    core::RenderingInstance &renderingInstance, std::set<star::Rendering_Features> &engineRenderingFeatures,
    std::set<Rendering_Device_Features> &engineRenderingDeviceFeatures)
{
    return core::device::StarDevice(renderingInstance, engineRenderingFeatures, engineRenderingDeviceFeatures, nullptr);
}

vk::Extent2D star::policy::DefaultEngineInitPolicy::getEngineRenderingResolution()
{
    return vk::Extent2D(1280, 720);
}

common::FrameTracker::Setup star::policy::DefaultEngineInitPolicy::getFrameInFlightTrackingSetup(
    core::device::StarDevice &device)
{
    return common::FrameTracker::Setup(0, 1);
}

std::vector<service::Service> star::policy::DefaultEngineInitPolicy::getAdditionalDeviceServices()
{
    return std::vector<service::Service>();
}

// service::Service star::policy::DefaultEngineInitPolicy::createFrameInFlightControllerService()
// {
//     return service::Service();
// }

core::RenderingInstance DefaultEngineInitPolicy::createRenderingInstance(std::string appName)
{
    std::vector<const char *> ext{};

    return {appName, ext};
}

} // namespace star::policy