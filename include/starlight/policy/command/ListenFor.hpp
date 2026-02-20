#pragma once

#include "starlight/core/CommandBus.hpp"

#include <utility>

namespace star::policy::command
{
template <typename TOwner, typename TCommand, auto TTypeNameProvider, auto THandler> class ListenFor
{
  public:
    explicit ListenFor(TOwner &me) : m_me(me)
    {
    }
    ListenFor(const ListenFor<TOwner, TCommand, TTypeNameProvider, THandler> &) = delete;
    ListenFor &operator=(const ListenFor<TOwner, TCommand, TTypeNameProvider, THandler> &) = delete;
    ListenFor(ListenFor<TOwner, TCommand, TTypeNameProvider, THandler> &&) = delete;
    ListenFor &operator=(ListenFor<TOwner, TCommand, TTypeNameProvider, THandler> &&) = delete;
    ~ListenFor() = default;

    void init(core::CommandBus &bus)
    {
        const auto type = bus.registerCommandType(TTypeNameProvider());
        bus.registerServiceCallback(
            type, star::common::ServiceCallback{this, [](void *ctx, star::common::IServiceCommand &base) {
                                                    auto *self = static_cast<ListenFor *>(ctx);
                                                    auto &cmd = static_cast<TCommand &>(base); // safe by type id
                                                    // Works with both non-const and const member functions
                                                    std::invoke(THandler, self->m_me, cmd);
                                                }});
    }

    void cleanup(core::CommandBus &bus)
    {
        const auto type = bus.registerCommandType(TTypeNameProvider());
        bus.removeServiceCallback(type);
    }

  private:
    TOwner &m_me;
};
} // namespace star::policy::command