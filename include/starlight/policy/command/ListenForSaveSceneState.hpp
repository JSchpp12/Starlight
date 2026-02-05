#pragma once

#include "starlight/command/SaveSceneState.hpp"
#include "starlight/core/CommandBus.hpp"

#include <concepts>

namespace star::policy
{
template <typename T> class ListenForSaveSceneState
{
  public:
    explicit ListenForSaveSceneState(T &me) : m_me(me)
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
        const auto type = commandBus.registerCommandType(star::command::save_scene_state::GetSaveSceneStateCommandTypeName);
        if (commandBus.getRegistry().contains(type))
        {
            commandBus.removeServiceCallback(type);
        }
    }

    void registerListener(core::CommandBus &commandBus)
    {
        const auto type = commandBus.registerCommandType(star::command::save_scene_state::GetSaveSceneStateCommandTypeName);

        commandBus.registerServiceCallback(
            type, star::common::ServiceCallback{this, [](void *ctx, star::common::IServiceCommand &base) {
                                                    auto *self = static_cast<ListenForSaveSceneState<T> *>(ctx);
                                                    auto &cmd = static_cast<command::SaveSceneState &>(base);

                                                    self->m_me.onSaveSceneState(cmd);
                                                }});
    }
};
} // namespace star::policy