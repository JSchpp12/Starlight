#include "tasks/TaskFactory.hpp"

#include "Compiler.hpp"
#include "StarShader.hpp"
#include "complete_tasks/CompleteTask.hpp"
#include "complete_tasks/TaskFactory.hpp"


#include "FileHelpers.hpp"

#include <memory>

star::job::tasks::Task<> star::job::tasks::task_factory::createPrintTask(std::string message)
{
    return star::job::tasks::Task<>::Builder<PrintPayload>()
        .setPayload(PrintPayload(std::move(message)))
        .setExecute([](void *p) {
            auto *data = static_cast<PrintPayload *>(p);
            std::cout << data->message << std::endl;
        })
        .build();
}

// void star::job::tasks::task_factory::MoveCompilePaylod(void *fresh, void *old)
// {
//     auto *o = static_cast<CompileShaderPayload *>(old);

//     new (fresh) CompileShaderPayload(std::move(*o));
//     o->finalizedShaderObject = nullptr;
//     o->compiledShaderCode = nullptr;
//     o->~CompileShaderPayload();
// }

// void star::job::tasks::task_factory::DestroyCompilePayload(void *p)
// {
//     auto *payload = static_cast<CompileShaderPayload *>(p);

//     // if (payload->finalizedShaderObject != nullptr){
//     //     payload
//     // }
//     // if (payload->compiledShaderCode != nullptr){
//     //     static_cast<std::unique_ptr<std::vector<uint32_t>> *>(payload->compiledShaderCode)->reset();
//     //     payload->compiledShaderCode = nullptr;
//     // }

//     payload->~CompileShaderPayload();
// }

std::optional<star::job::complete_tasks::CompleteTask<>> star::job::tasks::task_factory::CreateCompileComplete(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    auto complete = job::complete_tasks::task_factory::CreateShaderCompileComplete(
        data->handleID, std::move(data->finalizedShaderObject), std::move(data->compiledShaderCode));
    // data->finalizedShaderObject = nullptr;
    data->compiledShaderCode = nullptr;

    return std::make_optional<star::job::complete_tasks::CompleteTask<>>(std::move(complete));
}

void star::job::tasks::task_factory::ExecuteCompileShader(void *p)
{
    auto *data = static_cast<CompileShaderPayload *>(p);

    data->finalizedShaderObject = std::make_unique<StarShader>(data->path, data->stage);
    std::cout << "Comiling shader: " << data->path << std::endl;
    data->compiledShaderCode = star::Compiler::compile(data->path, true);
}

star::job::tasks::Task<> star::job::tasks::task_factory::CreateCompileShader(const std::string &fileName,
                                                                             const star::Shader_Stage &stage,
                                                                             const Handle &shaderHandle)
{
    return star::job::tasks::Task<>::Builder<CompileShaderPayload>()
        .setPayload(CompileShaderPayload{
            .path = fileName,
            .stage = stage,
            .handleID = shaderHandle.getID(),
        })
        // .setDestroy(&DestroyCompilePayload)
        // .setMovePayload(&MoveCompilePaylod)
        .setExecute(&ExecuteCompileShader)
        .setCreateCompleteTaskFunction(&CreateCompileComplete)
        .build();
}