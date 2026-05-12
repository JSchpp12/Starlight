#pragma once

#include "starlight/command/CreateObject.hpp"
#include "starlight/policy/command/ListenForCreateLight.hpp"
#include "starlight/policy/command/ListenForCreateObject.hpp"
#include "starlight/policy/command/ListenForSaveSceneState.hpp"
#include "starlight/service/InitParameters.hpp"
#include "starlight/service/detail/scene_loader/SceneObjectTracker.hpp"

namespace star::service
{
class SceneLoaderService
{
  public:
    explicit SceneLoaderService(std::string sceneFilePath);
    SceneLoaderService(const SceneLoaderService &) = delete;
    SceneLoaderService &operator=(const SceneLoaderService &) = delete;
    SceneLoaderService(SceneLoaderService &&other) noexcept;
    SceneLoaderService &operator=(SceneLoaderService &&other) noexcept;
    ~SceneLoaderService() = default;
    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
    }

    void init();

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void cleanup();

    void onCreateObject(command::CreateObject &cmd);

    void onSaveSceneState(command::SaveSceneState &cmd);

    void onCreateLight(command::CreateLight &cmd);

  private:
    std::string m_sceneFilePath;
    absl::flat_hash_map<std::string, std::shared_ptr<StarObject>> m_objectTracker;
    absl::flat_hash_map<std::string, std::shared_ptr<std::vector<star::Light>>> m_lightTracker;
    policy::ListenForCreateObject<SceneLoaderService> m_onCreate;
    policy::ListenForSaveSceneState<SceneLoaderService> m_onSceneSave;
    policy::command::ListenForCreateLight<SceneLoaderService> m_onCreateLight;
    star::core::CommandBus *m_deviceCommandBus = nullptr;

    void initListeners(core::CommandBus &commandBus) noexcept;

    void cleanupListeners(core::CommandBus &commandBus) noexcept;
};
} // namespace star::service