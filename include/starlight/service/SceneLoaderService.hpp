#pragma once

#include "starlight/command/CreateObject.hpp"
#include "starlight/policy/command/ListenForCreateObject.hpp"
#include "starlight/policy/command/ListenForSaveSceneState.hpp"
#include "starlight/service/InitParameters.hpp"
#include "starlight/service/detail/scene_loader/SceneObjectTracker.hpp"

namespace star::service
{
class SceneLoaderService
{
  public:
    SceneLoaderService();
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

    void onSaveSceneState(command::SaveSceneState &event);

  private:
    scene_loader::SceneObjectTracker m_objectTracker;
    policy::ListenForCreateObject<SceneLoaderService> m_onCreate;
    policy::ListenForSaveSceneState<SceneLoaderService> m_onSceneSave;
    star::core::CommandBus *m_deviceCommandBus = nullptr;

    void registerCommands(core::CommandBus &commandBus) noexcept;

    void cleanupCommands(core::CommandBus &commandBus) noexcept;
};
} // namespace star::service