#pragma once

#include "starlight/command/CreateObject.hpp"
#include "starlight/core/CommandBus.hpp"

#include <concepts>

namespace star::policy
{
template <typename T> class ListenForCreateObject
{
  public:
    explicit ListenForCreateObject(T &me) : m_me(me)
    {
    }

    void init(core::CommandBus &commandBus)
    {
        registerListener(commandBus);
    }

    void cleanup(core::CommandBus &commandBus)
    {
        cleanupListener(commandBus);
    }

  private:
    T &m_me;

    void cleanupListener(core::CommandBus &commandBus)
    {
        auto type = commandBus.registerCommandType(star::command::create_object::GetCreateObjectCommandTypeName);
        if (commandBus.getRegistry().contains(type))
        {
            commandBus.removeServiceCallback(type);
        }
    }

    void registerListener(core::CommandBus &commandBus)
    {
        auto type = commandBus.registerCommandType(star::command::create_object::GetCreateObjectCommandTypeName);

        commandBus.registerServiceCallback(
            type, star::common::ServiceCallback{this, [](void *ctx, star::common::IServiceCommand &base) {
                                                    auto *self = static_cast<ListenForCreateObject<T> *>(ctx);
                                                    auto &cmd = static_cast<command::CreateObject &>(base);

                                                    self->m_me.onCreateObject(cmd);
                                                }});
    }
};
} // namespace star::policy