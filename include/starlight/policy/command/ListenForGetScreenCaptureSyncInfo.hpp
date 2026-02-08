#pragma once

#include "starlight/command/GetScreenCaptureSyncInfo.hpp"
#include "starlight/core/CommandBus.hpp"

#include <concepts>

namespace star::policy
{
template <typename T>
concept ListenForGetSyncInfoLike = requires(T t, star::command::GetScreenCaptureSyncInfo &cmd) {
    { t.onGetSyncInfo(cmd) } -> std::same_as<void>;
};

template <typename T> class ListenForGetScreenCaptureSyncInfo
{
  public:
    explicit ListenForGetScreenCaptureSyncInfo(T &me) : m_me(me)
    {
    }
    ListenForGetScreenCaptureSyncInfo(const ListenForGetScreenCaptureSyncInfo &) = delete;
    ListenForGetScreenCaptureSyncInfo &operator=(const ListenForGetScreenCaptureSyncInfo &) = delete;
    ListenForGetScreenCaptureSyncInfo(ListenForGetScreenCaptureSyncInfo &&) = delete;
    ListenForGetScreenCaptureSyncInfo &operator=(ListenForGetScreenCaptureSyncInfo &&) = delete;
    ~ListenForGetScreenCaptureSyncInfo() = default;

    void init(core::CommandBus &bus)
    {
        registerListener(bus);
    }

    void cleanup(core::CommandBus &bus)
    {
        cleanupListener(bus);
    }

  private:
    T &m_me;

    void cleanupListener(core::CommandBus &bus)
    {
        const auto type = bus.registerCommandType(star::command::get_sync_info::GetSyncInfoCommandTypeName);
        bus.removeServiceCallback(type);
    }

    void registerListener(core::CommandBus &bus)
        requires ListenForGetSyncInfoLike<T>
    {
        const auto type = bus.registerCommandType(star::command::get_sync_info::GetSyncInfoCommandTypeName);

        bus.registerServiceCallback(type, star::common::ServiceCallback{
                                              this, [](void *ctx, star::common::IServiceCommand &base) {
                                                  auto *self = static_cast<ListenForGetScreenCaptureSyncInfo<T> *>(ctx);
                                                  auto &cmd = static_cast<star::command::GetScreenCaptureSyncInfo &>(base);

                                                  self->m_me.onGetSyncInfo(cmd);
                                              }});
    }
};
} // namespace star::policy