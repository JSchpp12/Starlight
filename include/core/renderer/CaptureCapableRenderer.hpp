#pragma once

#include "Renderer.hpp"
#include "core/command_buffer/ScreenCapture.hpp"

#include <string>

namespace star::core::renderer
{
class CaptureCapableRenderer : public Renderer
{
  public:
    CaptureCapableRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                           std::shared_ptr<std::vector<Light>> lights, std::shared_ptr<StarCamera> camera,
                           std::vector<std::shared_ptr<StarObject>> objects)
        : Renderer(context, numFramesInFlight, std::move(lights), std::move(camera), std::move(objects))
    {
    }

    CaptureCapableRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                           std::vector<std::shared_ptr<StarObject>> objects,
                           std::shared_ptr<ManagerController::RenderResource::Buffer> lightData,
                           std::shared_ptr<ManagerController::RenderResource::Buffer> lightListData,
                           std::shared_ptr<ManagerController::RenderResource::Buffer> cameraData)
        : Renderer(context, numFramesInFlight, std::move(objects), std::move(lightData), std::move(lightListData),
                   std::move(cameraData)) {};

    virtual ~CaptureCapableRenderer() = default;

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight) override;

    virtual void cleanupRender(core::device::DeviceContext &context);

    virtual void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex) override;

    void triggerCapture(std::string_view imageName);

  private:
    core::command_buffer::ScreenCapture m_screenCaptureCommands;

    core::command_buffer::ScreenCapture createScreenCaptureCommands(core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    void createWorker(core::device::DeviceContext &context);

};
} // namespace star::core::renderer