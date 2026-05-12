#include "starlight/policy/DefaultEngineInitPolicy.hpp"

#include "starlight/common/ConfigFile.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/job/tasks/IOTask.hpp"
#include "starlight/job/worker/detail/default_worker/SleepWaitTaskHandlingPolicy.hpp"
#include "starlight/service/CommandOrderService.hpp"
#include "starlight/service/FrameInFlightControllerService.hpp"
#include "starlight/service/HeadlessRenderResultWriteService.hpp"
#include "starlight/service/IOService.hpp"
#include "starlight/service/SceneLoaderService.hpp"
#include "starlight/service/ScreenCapture.hpp"
#include "starlight/service/detail/screen_capture/CopyDirectorPolicy.hpp"
#include "starlight/service/detail/screen_capture/CreateDependenciesPolicies.hpp"
#include "starlight/service/detail/screen_capture/WorkerControllerPolicies.hpp"

#include <string>

#include <star_common/helper/CastHelpers.hpp>

namespace star::policy
{
void DefaultEngineInitPolicy::init(uint8_t requestedNumFramesInFLight)
{
    m_maxNumFramesInFlight = std::move(requestedNumFramesInFLight);
}

void DefaultEngineInitPolicy::cleanup(core::RenderingInstance &instance)
{
}

star::core::device::StarDevice star::policy::DefaultEngineInitPolicy::createNewDevice(
    core::RenderingInstance &renderingInstance, std::set<star::Rendering_Features> &engineRenderingFeatures,
    std::set<Rendering_Device_Features> &engineRenderingDeviceFeatures)
{
    return core::device::StarDevice(renderingInstance, engineRenderingFeatures, engineRenderingDeviceFeatures, {},
                                    nullptr);
}

vk::Extent2D star::policy::DefaultEngineInitPolicy::getEngineRenderingResolution()
{
    return vk::Extent2D()
        .setWidth(static_cast<uint32_t>(star::ConfigFile::getInt(Config_Settings::resolution_x, 1920)))
        .setHeight(static_cast<uint32_t>(star::ConfigFile::getInt(Config_Settings::resolution_y, 1080)));
}

common::FrameTracker::Setup star::policy::DefaultEngineInitPolicy::getFrameInFlightTrackingSetup(
    core::device::StarDevice &device)
{
    return common::FrameTracker::Setup(m_maxNumFramesInFlight, m_maxNumFramesInFlight);
}

std::vector<service::Service> star::policy::DefaultEngineInitPolicy::getAdditionalDeviceServices()
{
    std::vector<service::Service> services = std::vector<service::Service>(6);
    services[0] = createFrameInFlightControllerService();
    services[1] = createIOService();
    services[2] = createCommandOrderService();
    services[3] = createScreenCaptureService();
    services[4] = createHeadlessCaptureService();
    services[5] = createSceneLoaderService();

    auto addServices = addAdditionalServices();
    for (size_t i{0}; i < addServices.size(); i++)
    {
        services.emplace_back(std::move(addServices[i]));
    }

    return services;
}

service::Service DefaultEngineInitPolicy::createScreenCaptureService()
{
    uint32_t maxWorkers = star::ConfigFile::getUint32(star::Config_Settings::max_image_worker_count, 8);

    return service::Service{service::ScreenCapture{
        service::detail::screen_capture::WorkerControllerPolicy{},
        service::detail::screen_capture::DefaultCreatePolicy{},
        service::detail::screen_capture::DefaultCopyPolicy{},
        maxWorkers}};
}

service::Service DefaultEngineInitPolicy::createIOService()
{
    return service::Service{service::IOService()};
}

service::Service DefaultEngineInitPolicy::createSceneLoaderService()
{
    return service::Service{service::SceneLoaderService(star::ConfigFile::getString(
        star::Config_Settings::scene_file, "default_scene"))};
}

service::Service DefaultEngineInitPolicy::createFrameInFlightControllerService()
{
    return service::Service{service::FrameInFlightControllerService{}};
}

service::Service DefaultEngineInitPolicy::createHeadlessCaptureService()
{
    return service::Service{service::HeadlessRenderResultWriteService{}};
}

service::Service DefaultEngineInitPolicy::createCommandOrderService()
{
    return service::Service{service::CommandOrderService()};
}

core::RenderingInstance DefaultEngineInitPolicy::createRenderingInstance(std::string appName)
{
    std::vector<const char *> ext{};

    return {appName, ext};
}

} // namespace star::policy