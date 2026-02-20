#pragma once

#include "starlight/command/FileIO/WriteToFile.hpp"
#include "starlight/core/CommandBus.hpp"

#include <concepts>

namespace star::policy
{
template <typename T>
concept ListenForWriteToFileLike = requires(T listener, star::command::file_io::WriteToFile &event) {
    { listener.onWriteToFile(event) } -> std::same_as<void>;
};
template <typename T> class ListenForWriteToFile
{
    T &m_me;

    uint16_t getType(core::CommandBus &bus) const
    {
        return bus.registerCommandType(star::command::file_io::write_to_file::GetWriteToFileCommandTypeName);
    }

    void cleanupListener(core::CommandBus &bus)
    {
        const auto type = getType(bus);
        if (bus.getRegistry().contains(type))
        {
            bus.removeServiceCallback(type);
        }
    }

    void registerListener(core::CommandBus &bus)
    {
        const auto type = getType(bus);

        bus.registerServiceCallback(
            type, star::common::ServiceCallback{this, [](void *ctx, star::common::IServiceCommand &base) {
                                                    auto *self = static_cast<ListenForWriteToFile<T> *>(ctx);
                                                    auto &cmd = static_cast<star::command::file_io::WriteToFile &>(base);

                                                    self->m_me.onWriteToFile(cmd);
                                                }});
    }

  public:
    explicit ListenForWriteToFile(T &me) : m_me(me)
    {
    }

    void init(core::CommandBus &bus)
        requires ListenForWriteToFileLike<T>
    {
        registerListener(bus);
    }

    void cleanup(core::CommandBus &bus)
    {
        cleanupListener(bus);
    }
};
} // namespace star::policy