#include "starlight/policy/DefaultEngineInitPolicy.hpp"

#include "starlight/common/ConfigFile.hpp"

#include <string>

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
    core::RenderingInstance &renderingInstance, std::set<Rendering_Device_Features> &engineRenderingDeviceFeatures)
{
    const auto startupDeviceRequirements = consumeStartupDeviceRequirements();

    auto builder = core::device::StarDevice::Builder(renderingInstance)
                       .setRenderingDeviceFeatures(engineRenderingDeviceFeatures)
                       .addRequiredDeviceRequirements(startupDeviceRequirements);

    const int overridenEngineID =
        m_overrideRenderingDeviceIndex.has_value()
            ? m_overrideRenderingDeviceIndex.value()
            : star::ConfigFile::getInt(star::Config_Settings::required_device_feature_gpu_index, -1);
    if (overridenEngineID != -1)
        builder.setOverrideDeviceID(overridenEngineID);

    return builder.build();
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
    return m_engineServices.takeAll();
}

core::RenderingInstance DefaultEngineInitPolicy::createRenderingInstance(std::string appName)
{
    std::vector<const char *> ext{};

    return {appName, ext};
}

} // namespace star::policy
