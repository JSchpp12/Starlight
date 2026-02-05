#include "starlight/core/CommandBus.hpp"

#include <cassert>

namespace star::core
{

uint16_t CommandBus::registerCommandType(std::string_view typeName)
{
    return m_types.registerType(typeName);
}

void CommandBus::submit(star::common::IServiceCommand &command) const noexcept
{
    assert(m_serviceCallbacks.contains(command.getType()) && "Command does not have registered callback");

    m_serviceCallbacks.at(command.getType())(command);
}

void CommandBus::removeServiceCallback(const uint16_t &commandType)
{
    assert(m_serviceCallbacks.contains(commandType) &&
           "Attempted to delete service callback for type which does not exist in container");

    m_serviceCallbacks.erase(commandType);
}

void CommandBus::registerServiceCallback(const uint16_t &commandType, star::common::ServiceCallback callback)
{
    assert(!m_serviceCallbacks.contains(commandType) &&
           "Command already has registerd callback. Only one callback per type is allowed");

    m_serviceCallbacks.insert(std::pair<uint16_t, common::ServiceCallback>(commandType, std::move(callback)));
}

} // namespace star::core