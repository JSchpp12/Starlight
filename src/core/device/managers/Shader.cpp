#include "device/managers/Shader.hpp"

#include "device/system/event/ShaderCompiled.hpp"
#include "job/tasks/task_factory/TaskFactory.hpp"

void star::core::device::manager::Shader::submitTask(const Handle &handle, device::StarDevice &device,
                                                     job::TaskManager &taskSystem, system::EventBus &eventBus,
                                                     ShaderRecord *storedRecord)
{

    taskSystem.submitTask(job::tasks::task_factory::compile_shader::Create(storedRecord->request.shader.getPath(),
                                                                           storedRecord->request.shader.getStage(), handle,
                                                                           std::move(storedRecord->request.compiler)));
}