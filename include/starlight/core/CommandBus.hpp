#pragma once

#include <absl/container/flat_hash_map.h>
#include <star_common/Handle.hpp>
#include <star_common/IServiceCommand.hpp>
#include <star_common/ServiceCallbackInfo.hpp>
#include <star_common/TypeRegistry.hpp>

#include <string_view>

namespace star::core
{

class CommandBus
{
  public:
    void submit(star::common::IServiceCommand &command) const noexcept;
    void registerServiceCallback(const uint16_t &commandType, star::common::ServiceCallback callback);
    void removeServiceCallback(const uint16_t &commandType);
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