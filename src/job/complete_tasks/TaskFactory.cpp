#include "complete_tasks/TaskFactory.hpp"

#include "device/StarDevice.hpp"
#include "job/tasks/TaskFactory.hpp"

#pragma region CompileShaders
#include "core/device/StarDevice.hpp"
#include "core/device/managers/ManagerShader.hpp"

// void star::job::complete_tasks::task_factory::MoveShaderCompletePaylod(void *fresh, void *old)
// {
//     auto *o = static_cast<CompileCompletePayload *>(old);

//     new (fresh) CompileCompletePayload(std::move(*o));
//     o->finalizedShaderObject = nullptr;
//     o->compiledShaderCode = nullptr;
//     o->~CompileCompletePayload();
// }

// void star::job::complete_tasks::task_factory::DestroyShaderCompletePayload(void *object)
// {
//     auto *o = static_cast<CompileCompletePayload *>(object);

//     if (o->finalizedShaderObject != nullptr)
//     {
//         delete static_cast<StarShader *>(o->finalizedShaderObject);
//     }
//     if (o->compiledShaderCode != nullptr)
//     {
//         static_cast<std::unique_ptr<std::vector<uint32_t>> *>(o->compiledShaderCode)->reset();
//         o->compiledShaderCode = nullptr;
//     }

//     o->~CompileCompletePayload();
// }

void star::job::complete_tasks::task_factory::ExecuteShaderCompileComplete(void *device, void *shaderManager,
                                                                           void *payload)
{
    auto *d = static_cast<core::device::StarDevice *>(device);
    auto *sm = static_cast<star::core::device::manager::Shader *>(shaderManager);
    auto *p = static_cast<CompileCompletePayload *>(payload);

    Handle shader = Handle();
    shader.setID(p->handleID);
    shader.setType(Handle_Type::shader);

    assert(p->compiledShaderCode != nullptr && "Compiled shader data not properly set");

    std::cout << "Marking shader at index [" << p->handleID << "] as ready" << std::endl;

    sm->get(shader).compiledShader = std::move(p->compiledShaderCode);
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