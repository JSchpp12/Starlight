#include "device/managers/Shader.hpp"

#include "device/system/event/ShaderCompiled.hpp"
#include "job/tasks/TaskFactory.hpp"

void star::core::device::manager::Shader::submitTask(device::StarDevice &device, const Handle &handle,
                                                     job::TaskManager &taskSystem, common::EventBus &eventBus,
                                                     ShaderRecord *storedRecord)
{
    taskSystem.submitTask(job::tasks::compile_shader::Create(
                              std::move(storedRecord->request.path), storedRecord->request.shader.getStage(), handle,
                              device.getVulkanDevice(), std::move(storedRecord->request.macroDefinitions)),
                          job::tasks::compile_shader::CompileShaderTypeName);
}