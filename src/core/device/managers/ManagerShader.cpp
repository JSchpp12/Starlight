#include "device/managers/ManagerShader.hpp"

#include "job/tasks/TaskFactory.hpp"
#include "device/system/ShaderCompiledEvent.hpp"

void star::core::device::manager::Shader::submitTask(const Handle &handle, device::StarDevice &device, job::TaskManager &taskSystem,
                                                     system::EventBus &eventBus, ShaderRecord *storedRecord)
{

    taskSystem.submitTask(job::tasks::task_factory::CreateCompileShader(storedRecord->shader.getPath(),
                                                                        storedRecord->shader.getStage(), handle));
}