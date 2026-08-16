#include "renderer/DefaultRenderPhaseProvider.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"
#include "ManagerController_RenderResource_LightList.hpp"
#include "core/device/DeviceContext.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <memory>
#include <utility>

namespace star::core::renderer
{
static std::shared_ptr<FrameData> CreateDefaultFrameData(core::device::DeviceContext &context,
                                                         std::shared_ptr<std::vector<Light>> lights,
                                                         std::shared_ptr<StarCamera> camera)
{
    auto cameraController = std::make_shared<ManagerController::RenderResource::GlobalInfo>(camera);
    auto lightInfoController = std::make_shared<ManagerController::RenderResource::LightInfo>(
        context.frameTracker().getSetup().getNumFramesInFlight(), lights);
    auto lightListController = std::make_shared<ManagerController::RenderResource::LightList>(
        context.frameTracker().getSetup().getNumFramesInFlight(), lights);

    auto fd = std::make_shared<FrameData>();
    fd->add(std::move(cameraController), roleHandle(frame_roles::Camera))
        .add(std::move(lightInfoController), roleHandle(frame_roles::LightInfo))
        .add(std::move(lightListController), roleHandle(frame_roles::LightList));

    return fd;
}

DefaultRenderPhaseProvider::DefaultRenderPhaseProvider(core::device::DeviceContext &context,
                                                       std::shared_ptr<std::vector<Light>> lights,
                                                       std::shared_ptr<StarCamera> camera,
                                                       std::vector<std::shared_ptr<StarObject>> objects)
    : m_objects(std::move(objects)), m_frameData(CreateDefaultFrameData(context, lights, camera)),
      m_createdFrameData(true)
{
}

DefaultRenderPhaseProvider::DefaultRenderPhaseProvider(core::device::DeviceContext &context,
                                                       std::vector<std::shared_ptr<StarObject>> objects,
                                                       std::shared_ptr<FrameData> frameData)
    : m_objects(std::move(objects)), m_frameData(std::move(frameData)), m_createdFrameData(false)
{
}

std::unique_ptr<RenderPhase> DefaultRenderPhaseProvider::build(core::device::DeviceContext &device,
                                                               RenderPhaseRegistry & /*phases*/)
{
    return DefaultRenderPhase::Builder(device)
        .setObjects(std::move(m_objects))
        .setFrameData(m_frameData)
        .setDataRoles(roleHandle(frame_roles::Camera), roleHandle(frame_roles::LightInfo),
                      roleHandle(frame_roles::LightList), m_createdFrameData)
        .setConfig(m_config)
        .buildUnique();
}
} // namespace star::core::renderer