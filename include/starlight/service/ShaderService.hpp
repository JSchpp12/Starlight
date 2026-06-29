#pragma once

#include "starlight/core/WorkerPool.hpp"
#include "starlight/core/device/managers/GraphicsContainer.hpp"
#include "starlight/job/TaskManager.hpp"
#include "starlight/policy/command/ListenForLoadShader.hpp"
#include "starlight/service/InitParameters.hpp"

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

    void onLoadShader(star::command::shader::LoadShader &cmd);

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
    }

  private:
    policy::command::ListenForLoadShader<ShaderService> m_listenForLoadShader;
    core::device::manager::GraphicsContainer *m_graphicsManagers = nullptr;
    core::CommandBus *m_cmdBus = nullptr;

    void initListeners(core::CommandBus &bus);

    void cleanupListeners(core::CommandBus &bus);
};
} // namespace star::service