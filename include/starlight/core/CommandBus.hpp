#pragma once

#include <absl/container/flat_hash_map.h>

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>
#include <star_common/ServiceCallbackInfo.hpp>
#include <star_common/TypeRegistry.hpp>

#include <cassert>
#include <string_view>

namespace star::core
{

namespace command_bus
{
template <typename TCmd> uint16_t ResolveTypeID(const common::TypeRegistry &registry)
{
    static const uint16_t cached = registry.getTypeGuaranteedExist(TCmd::GetUniqueTypeName());
    return cached;
}
} // namespace command_bus

class CommandBus
{
  public:
    template <typename TCmd> void submit(TCmd &command) const noexcept
    {
        const uint16_t typeID = command_bus::ResolveTypeID<TCmd>(m_types);

        command.setType(typeID);
        submitKnownType(static_cast<common::IServiceCommand &>(command));
    }

    void submitKnownType(common::IServiceCommand &command) const;
    void registerServiceCallback(uint16_t commandType, star::common::ServiceCallback callback);
    void removeServiceCallback(uint16_t commandType);
    uint16_t registerCommandType(std::string_view typeName);
    common::TypeRegistry &getRegistry()
    {
        return m_types;
    }

  private:
    absl::flat_hash_map<uint16_t, star::common::ServiceCallback> m_serviceCallbacks;
    star::common::TypeRegistry m_types;
};
} // namespace star::core