#include "starlight/core/CommandBus.hpp"

#include <cassert>

namespace star::core
{

void CommandBus::submitKnownType(common::IServiceCommand &command) const
{
    assert(m_serviceCallbacks.contains(command.getType()) &&
           "Command does not have registered callback");
    m_serviceCallbacks.at(command.getType())(command);
}

uint16_t CommandBus::registerCommandType(std::string_view typeName)
{
    return m_types.registerType(typeName);
}

void CommandBus::removeServiceCallback(uint16_t commandType)
{
    assert(m_serviceCallbacks.contains(commandType) &&
           "Attempted to delete service callback for type which does not exist in container");

    m_serviceCallbacks.erase(commandType);
}

void CommandBus::registerServiceCallback(uint16_t commandType, star::common::ServiceCallback callback)
{
    auto [it, ok] = m_serviceCallbacks.try_emplace(commandType, std::move(callback));
    assert(ok && "Command already has a registered callback");
}

} // namespace star::core