#include "starlight/maker/texture/MakeSkybox.hpp"

#include "starlight/command/FileIO/ReadFromFile.hpp"
#include "starlight/common/textures/TransferRequest_Skybox.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/DeviceContext.hpp"
#include "starlight/core/helper/queue/QueueHelpers.hpp"
#include "starlight/enums/Enums.hpp"
#include "starlight/job/tasks/IOTask.hpp"
#include "starlight/managers/ManagerRenderResource.hpp"
#include "starlight/structs/SkyboxInfo.hpp"
#include "starlight/util/io/SkyboxInfoReader.hpp"

#include <filesystem>
#include <future>
#include <memory>
#include <utility>

namespace star::maker::texture
{
static std::filesystem::path GetInfoPath(const std::string &texture)
{
    std::filesystem::path infoPath{texture};
    infoPath.replace_extension();
    infoPath += ".skyboxinfo.json";

    return infoPath;
}

MakeSkybox::MakeSkybox(core::device::DeviceContext &deviceContext) : m_deviceContext(deviceContext)
{
}

MakeSkybox &MakeSkybox::setTexture(std::string texture)
{
    m_texture = std::move(texture);
    return *this;
}

Handle MakeSkybox::make()
{
    if (m_texture.empty())
        STAR_THROW("Skybox texture must be set before make() is called");

    const auto *graphicsQueue = core::helper::GetEngineDefaultQueue(
        m_deviceContext.getEventBus(), m_deviceContext.getGraphicsManagers().queueManager, Queue_Type::Tgraphics);
    if (graphicsQueue == nullptr)
        STAR_THROW("Unable to find the engine graphics queue for the skybox transfer");

    const std::filesystem::path infoPath = GetInfoPath(m_texture);

    io::SkyboxInfoReader reader;
    auto skyboxInfo = std::make_shared<std::future<SkyboxInfo>>(reader.getFuture());

    auto readPayload =
        job::tasks::io::ReadPayload<io::SkyboxInfoReader>{.filePath = infoPath, .readFunction = std::move(reader)};
    auto readTask = job::tasks::io::CreateReadTask(std::move(readPayload));
    command::file_io::ReadFromFile readCmd{std::move(readTask)};
    m_deviceContext.getCmdBus().submit(readCmd);

    auto request =
        std::make_unique<TransferRequest::Skybox>(std::move(skyboxInfo), graphicsQueue->getParentQueueFamilyIndex());
    return ManagerRenderResource::addRequest(m_deviceContext.getDeviceID(), std::move(request));
}
} // namespace star::maker::texture
