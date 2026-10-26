#pragma once

#include "starlight/core/WorkerPool.hpp"
#include "starlight/core/device/managers/GraphicsContainer.hpp"
#include "starlight/job/TaskManager.hpp"
#include "starlight/policy/command/ListenForCreatePipeline.hpp"
#include "starlight/service/InitParameters.hpp"

namespace star::service
{
class PipelineCommandService
{
  public:
    PipelineCommandService();
    PipelineCommandService(const PipelineCommandService &) = delete;
    PipelineCommandService &operator=(const PipelineCommandService &) = delete;
    PipelineCommandService(PipelineCommandService &&other);
    PipelineCommandService &operator=(PipelineCommandService &&other);
    ~PipelineCommandService() = default;

    void init();

    void shutdown();

    void setInitParameters(InitParameters &params);

    void onCreatePipeline(star::command::pipeline::CreatePipeline &cmd);

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
    }

  private:
    policy::command::ListenForCreatePipeline<PipelineCommandService> m_listenForCreatePipeline;
    core::device::manager::GraphicsContainer *m_graphicsManagers = nullptr;
    core::CommandBus *m_cmdBus = nullptr;

    void initListeners(core::CommandBus &bus);

    void cleanupListeners(core::CommandBus &bus);
};
} // namespace star::service
