#include "starlight/service/SceneLoaderService.hpp"

#include "starlight/command/CreateObject.hpp"
#include "starlight/common/ConfigFile.hpp"
#include "starlight/service/detail/scene_loader/SceneFile.hpp"

namespace star::service
{
SceneLoaderService::SceneLoaderService()
    : m_objectTracker(), m_onCreate(*this), m_onSceneSave(*this)
{
}

SceneLoaderService::SceneLoaderService(SceneLoaderService &&other) noexcept
    : m_objectTracker(std::move(other.m_objectTracker)), m_onCreate(*this), m_onSceneSave(*this),
      m_deviceCommandBus(other.m_deviceCommandBus)
{

    if (m_deviceCommandBus != nullptr)
    {
        other.cleanupCommands(*m_deviceCommandBus);
        registerCommands(*m_deviceCommandBus);
    }
}

SceneLoaderService &SceneLoaderService::operator=(SceneLoaderService &&other) noexcept
{
    if (this != &other)
    {
        m_objectTracker = std::move(other.m_objectTracker);
        m_deviceCommandBus = other.m_deviceCommandBus;
        if (m_deviceCommandBus != nullptr)
        {
            other.cleanupCommands(*m_deviceCommandBus);
            registerCommands(*m_deviceCommandBus);
        }
    }

    return *this;
}

void SceneLoaderService::init()
{
    assert(m_deviceCommandBus != nullptr);

    registerCommands(*m_deviceCommandBus);
}

void SceneLoaderService::setInitParameters(star::service::InitParameters &params)
{
    m_deviceCommandBus = &params.commandBus;
}

void SceneLoaderService::shutdown()
{
    if (m_deviceCommandBus != nullptr)
    {
        cleanupCommands(*m_deviceCommandBus);
    }
}

void SceneLoaderService::cleanup(common::EventBus &eventBus)
{
    (void)eventBus;

    if (m_deviceCommandBus != nullptr)
    {
        cleanupCommands(*m_deviceCommandBus);
    }
}

void SceneLoaderService::cleanupCommands(core::CommandBus &commandBus) noexcept
{
    m_onCreate.cleanup(commandBus);
    m_onSceneSave.cleanup(commandBus);
}

void SceneLoaderService::onCreateObject(command::CreateObject &event)
{
    const auto &uniqueName = event.getUniqueName();
    assert(!m_objectTracker.contains(uniqueName));

    auto newObject = event.load();
    m_objectTracker.insert(std::make_pair(uniqueName, newObject));

    auto file = scene_loader::SceneFile("StarScene.json");
    auto fileData = file.tryReadObjectInfo(uniqueName); 
    newObject->createInstance();

    if (fileData.has_value()){
        newObject->getInstance().setPosition(fileData.value().position); 
        newObject->getInstance().setScale(fileData.value().scale); 
        for (const auto &rot : fileData.value().rotationsToApply){
            newObject->getInstance().rotateRelative(rot.first, rot.second, true);
        }
    }

    event.getReply().set(newObject);
}

void SceneLoaderService::onSaveSceneState(command::SaveSceneState &event)
{
    (void)event;
    auto file = scene_loader::SceneFile("StarScene.json"); 
    file.write(m_objectTracker);
}

void SceneLoaderService::registerCommands(core::CommandBus &commandBus) noexcept
{
    m_onCreate.init(commandBus);
    m_onSceneSave.init(commandBus);
}
} // namespace star::service