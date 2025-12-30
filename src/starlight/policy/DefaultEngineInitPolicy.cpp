#include "starlight/policy/DefaultEngineInitPolicy.hpp"

#include "starlight/common/ConfigFile.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/service/FrameInFlightControllerService.hpp"
#include "starlight/service/HeadlessRenderResultWriteService.hpp"

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
    uint32_t width;
    {
        int w{std::stoi(star::ConfigFile::getSetting(Config_Settings::resolution_x))};
        if (!star::common::helper::SafeCast(w, width))
        {
            STAR_THROW("Failed to parse and process config setting resolution_x");
        }
    }

    uint32_t height;
    {
        int h{std::stoi(star::ConfigFile::getSetting(Config_Settings::resolution_y))};
        if (!star::common::helper::SafeCast(h, height))
        {
            STAR_THROW("Failed to parse and process config settings resolution_y");
        }
    }

    return vk::Extent2D().setWidth(width).setHeight(height);
}

common::FrameTracker::Setup star::policy::DefaultEngineInitPolicy::getFrameInFlightTrackingSetup(
    core::device::StarDevice &device)
{
    return common::FrameTracker::Setup(m_maxNumFramesInFlight, m_maxNumFramesInFlight);
}

std::vector<service::Service> star::policy::DefaultEngineInitPolicy::getAdditionalDeviceServices()
{
    std::vector<service::Service> services;
    services.emplace_back(createFrameInFlightControllerService());
    services.emplace_back(createHeadlessCaptureService());
    return services;
}

service::Service DefaultEngineInitPolicy::createFrameInFlightControllerService() const
{
    return service::Service{service::FrameInFlightControllerService{}};
}

service::Service DefaultEngineInitPolicy::createHeadlessCaptureService() const
{
    return service::Service{service::HeadlessRenderResultWriteService{}};
}

core::RenderingInstance DefaultEngineInitPolicy::createRenderingInstance(std::string appName)
{
    std::vector<const char *> ext{};

    return {appName, ext};
}

} // namespace star::policy