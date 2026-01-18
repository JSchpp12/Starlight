#pragma once

#include "starlight/command/CreateObject.hpp"
#include "starlight/service/InitParameters.hpp"

namespace star::service
{
class SceneLoaderService
{
  public:
    SceneLoaderService() = default;
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
    star::core::CommandBus *m_deviceCommandBus = nullptr;

    void registerCommands(core::CommandBus &commandBus) noexcept;

    void cleanupCommands(core::CommandBus &commandBus) noexcept;
};
} // namespace star::service