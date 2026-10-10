#pragma once

#include "starlight/service/EngineServices.hpp"
#include "starlight/service/Service.hpp"

namespace star::service
{
/// @brief Builds the set of services required by the core engine.
class EngineServicesFactory
{
  public:
    /// Presenting (windowed) configurations must pass false and then append the windowing-required services via
    /// star::windowing::EngineServicesFactory.
    static EngineServices createDefault(bool initHeadlessRequiredServices = true);

    static Service createFrameInFlightControllerService();
    static Service createIOService();
    static Service createCommandOrderService();
    static Service createScreenCaptureService();
    static Service createHeadlessRenderResultWriteService();
    static Service createSceneLoaderService();
    static Service createShaderService();
    static Service createPipelineCommandService();
};
} // namespace star::service
