#include "starlight/service/SceneLoaderService.hpp"

#include "starlight/command/CreateObject.hpp"

namespace star::service
{
SceneLoaderService::SceneLoaderService(SceneLoaderService &&other) noexcept
    :  m_onCreate(*this), m_deviceCommandBus(other.m_deviceCommandBus)
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
}

void SceneLoaderService::onCreateObject(command::CreateObject &event)
{
    const auto &uniqueName = event.getUniqueName();
    assert(!m_objectStates.contains(uniqueName));

    auto newObject = event.load();
    m_objectStates.insert(std::make_pair(uniqueName, newObject));

    event.getReply().set(newObject);
}

void SceneLoaderService::registerCommands(core::CommandBus &commandBus) noexcept
{
    m_onCreate.init(commandBus);
}
} // namespace star::service