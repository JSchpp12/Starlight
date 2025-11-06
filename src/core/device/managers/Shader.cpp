#include "device/managers/Shader.hpp"

#include "device/system/event/ShaderCompiled.hpp"
#include "job/tasks/TaskFactory.hpp"

void star::core::device::manager::Shader::submitTask(device::StarDevice &device, const Handle &handle,
                                                     job::TaskManager &taskSystem, system::EventBus &eventBus,
                                                     ShaderRecord *storedRecord)
{

    taskSystem.submitTask(job::tasks::compile_shader::Create(
        storedRecord->request.shader.getPath(), storedRecord->request.shader.getStage(), handle,
        std::move(storedRecord->request.compiler)));
}