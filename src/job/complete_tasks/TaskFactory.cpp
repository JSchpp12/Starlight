#include "complete_tasks/TaskFactory.hpp"

#include "device/StarDevice.hpp"
#include "job/tasks/TaskFactory.hpp"

#pragma region CompileShaders
#include "core/device/StarDevice.hpp"
#include "core/device/managers/ManagerShader.hpp"
#include "core/device/system/EventBus.hpp"
#include "core/device/system/ShaderCompiledEvent.hpp"

void star::job::complete_tasks::task_factory::ExecuteShaderCompileComplete(void *device, void *eventBus, void *shaderManager,
                                                                           void *payload)
{
    auto *d = static_cast<core::device::StarDevice *>(device);
    auto *sm = static_cast<star::core::device::manager::Shader *>(shaderManager);
    auto *eb = static_cast<star::core::device::system::EventBus *>(eventBus); 
    auto *p = static_cast<CompileCompletePayload *>(payload);

    Handle shader = Handle();
    shader.setID(p->handleID);
    shader.setType(Handle_Type::shader);

    eb->emit<core::device::system::ShaderCompiledEvent>(core::device::system::ShaderCompiledEvent(shader)); 

    assert(p->compiledShaderCode != nullptr && "Compiled shader data not properly set");

    std::cout << "Marking shader at index [" << p->handleID << "] as ready" << std::endl;

    sm->get(shader)->compiledShader = std::move(p->compiledShaderCode);
}

star::job::complete_tasks::CompleteTask<> star::job::complete_tasks::task_factory::CreateShaderCompileComplete(
    size_t handleID, std::unique_ptr<StarShader> finalizedShaderObject, std::unique_ptr<std::vector<uint32_t>> finalizedCompiledShader)
{
    return complete_tasks::CompleteTask<>::Builder<CompileCompletePayload>()
        .setPayload(CompileCompletePayload{.handleID = std::move(handleID),
                                           .finalizedShaderObject = std::move(finalizedShaderObject),
                                           .compiledShaderCode = std::move(finalizedCompiledShader)})
        .setEngineExecuteFunction(&ExecuteShaderCompileComplete)
        // .setMovePayloadFunction(&MoveShaderCompletePaylod)
        // .setDestroyFunction(&DestroyShaderCompletePayload)
        .build();
}

#pragma endregion CompileShaders