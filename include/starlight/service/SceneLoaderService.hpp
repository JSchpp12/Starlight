#pragma once

#include "starlight/command/CreateObject.hpp"
#include "starlight/policy/command/ListenForCreateObject.hpp"
#include "starlight/service/InitParameters.hpp"

#include <absl/container/flat_hash_map.h>

namespace star::service
{
class SceneLoaderService
{
  public:
    SceneLoaderService() : m_objectStates(), m_onCreate(*this){};
    SceneLoaderService(const SceneLoaderService &) = delete;
    SceneLoaderService &operator=(const SceneLoaderService &) = delete;
    SceneLoaderService(SceneLoaderService &&other) noexcept;
    SceneLoaderService &operator=(SceneLoaderService &&other) noexcept;
    ~SceneLoaderService() = default;

    void init();

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void cleanup(common::EventBus &eventBus);

    void onCreateObject(command::CreateObject &event);

  private:
    absl::flat_hash_map<std::string, std::shared_ptr<StarObject>> m_objectStates;
    policy::ListenForCreateObject<SceneLoaderService> m_onCreate;
    star::core::CommandBus *m_deviceCommandBus = nullptr;

    void registerCommands(core::CommandBus &commandBus) noexcept;

    void cleanupCommands(core::CommandBus &commandBus) noexcept;
};
} // namespace star::service