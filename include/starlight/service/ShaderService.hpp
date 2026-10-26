#pragma once

#include "starlight/core/WorkerPool.hpp"
#include "starlight/core/device/managers/GraphicsContainer.hpp"
#include "starlight/job/TaskManager.hpp"
#include "starlight/policy/command/ListenForCreateShader.hpp"
#include "starlight/service/InitParameters.hpp"
#include "starlight/util/io/PathResolver.hpp"

namespace star::service
{
class ShaderService
{
  public:
    ShaderService();
    ShaderService(const ShaderService &) = delete;
    ShaderService &operator=(const ShaderService &) = delete;
    ShaderService(ShaderService &&other);
    ShaderService &operator=(ShaderService &&other);
    ~ShaderService() = default;

    void init();

    void shutdown();

    void setInitParameters(InitParameters &params);

    void onCreateShader(star::command::shader::CreateShader &cmd);

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
    }

  private:
    policy::command::ListenForCreateShader<ShaderService> m_listenForCreateShader;
    core::device::manager::GraphicsContainer *m_graphicsManagers = nullptr;
    core::CommandBus *m_cmdBus = nullptr;
    const star::io::PathResolver *m_pathResolver = nullptr;

    void initListeners(core::CommandBus &bus);

    void cleanupListeners(core::CommandBus &bus);
};
} // namespace star::service