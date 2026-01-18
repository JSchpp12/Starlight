#include "starlight/service/SceneLoaderService.hpp"

#include "starlight/command/CreateObject.hpp"

namespace star::service
{
SceneLoaderService::SceneLoaderService(SceneLoaderService &&other) noexcept
    : m_deviceCommandBus(other.m_deviceCommandBus)
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
}

void SceneLoaderService::cleanupCommands(core::CommandBus &commandBus) noexcept
{
    if (m_deviceCommandBus->getRegistry().contains(star::command::create_object::GetCreateObjectCommandTypeName))
    {
        // need to remove first
        commandBus.removeServiceCallback(commandBus.getRegistry().getTypeGuaranteedExist(
            star::command::create_object::GetCreateObjectCommandTypeName));
    }
}

void SceneLoaderService::onCreateObject(command::CreateObject &event)
{
    std::cout << "Big test";
}

void SceneLoaderService::registerCommands(core::CommandBus &commandBus) noexcept
{
    auto type = m_deviceCommandBus->registerCommandType(star::command::create_object::GetCreateObjectCommandTypeName);

    m_deviceCommandBus->registerServiceCallback(
        type, star::common::ServiceCallback{this, [](void *ctx, star::common::IServiceCommand &base) {
                                                auto *self = static_cast<SceneLoaderService *>(ctx);
                                                auto &cmd = static_cast<command::CreateObject &>(base);

                                                self->onCreateObject(cmd);
                                            }});
}
} // namespace star::service