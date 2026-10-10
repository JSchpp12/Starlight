#include "starlight/service/EngineServicesFactory.hpp"

#include "starlight/common/ConfigFile.hpp"
#include "starlight/service/CommandOrderService.hpp"
#include "starlight/service/FrameInFlightControllerService.hpp"
#include "starlight/service/HeadlessRenderResultWriteService.hpp"
#include "starlight/service/IOService.hpp"
#include "starlight/service/PipelineCommandService.hpp"
#include "starlight/service/SceneLoaderService.hpp"
#include "starlight/service/ScreenCapture.hpp"
#include "starlight/service/ShaderService.hpp"
#include "starlight/service/detail/screen_capture/CopyDirectorPolicy.hpp"
#include "starlight/service/detail/screen_capture/CreateDependenciesPolicies.hpp"
#include "starlight/service/detail/screen_capture/WorkerControllerPolicies.hpp"

#include <string>
#include <utility>

namespace star::service
{
EngineServices EngineServicesFactory::createDefault(bool initHeadlessRequiredServices)
{
    std::vector<Service> services;
    services.reserve(initHeadlessRequiredServices ? 8 : 6);

    if (initHeadlessRequiredServices)
    {
        services.emplace_back(createFrameInFlightControllerService());
    }

    services.emplace_back(createIOService());
    services.emplace_back(createCommandOrderService());
    services.emplace_back(createScreenCaptureService());
    services.emplace_back(createSceneLoaderService());
    services.emplace_back(createShaderService());
    services.emplace_back(createPipelineCommandService());
    if (initHeadlessRequiredServices)
    {
        services.emplace_back(createHeadlessRenderResultWriteService());
    }

    return EngineServices{std::move(services)};
}

Service EngineServicesFactory::createScreenCaptureService()
{
    uint32_t maxWorkers = star::ConfigFile::getUint32(star::Config_Settings::max_image_worker_count, 2);

    return Service{ScreenCapture{detail::screen_capture::WorkerControllerPolicy{},
                                 detail::screen_capture::DefaultCreatePolicy{},
                                 detail::screen_capture::DefaultCopyPolicy{}, maxWorkers}};
}

Service EngineServicesFactory::createIOService()
{
    return Service{IOService()};
}

Service EngineServicesFactory::createSceneLoaderService()
{
    return Service{SceneLoaderService(star::ConfigFile::getString(star::Config_Settings::scene_file, "default_scene"))};
}

Service EngineServicesFactory::createFrameInFlightControllerService()
{
    return Service{FrameInFlightControllerService{}};
}

Service EngineServicesFactory::createHeadlessRenderResultWriteService()
{
    return Service{HeadlessRenderResultWriteService{}};
}

Service EngineServicesFactory::createCommandOrderService()
{
    return Service{CommandOrderService()};
}

Service EngineServicesFactory::createShaderService()
{
    return Service{ShaderService()};
}

Service EngineServicesFactory::createPipelineCommandService()
{
    // Registers the pipeline command types (e.g. pipeline::CreatePipeline) and
    // their callbacks on the device command bus. Without it, any
    // star::StarObject::buildPipeline() submission resolves an unregistered
    // type name and fails in CommandBus::submit().
    return Service{PipelineCommandService()};
}
} // namespace star::service
