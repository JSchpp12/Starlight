#include "tasks/task_factory/TaskFactory.hpp"

#include "Compiler.hpp"
#include "StarShader.hpp"
#include "complete_tasks/CompleteTask.hpp"


#include "FileHelpers.hpp"

#include <boost/filesystem.hpp>

#include <memory>

namespace star::job::tasks::task_factory
{
star::job::tasks::Task<> createPrintTask(std::string message)
{
    return star::job::tasks::Task<>::Builder<PrintPayload>()
        .setPayload(PrintPayload(std::move(message)))
        .setExecute([](void *p) {
            auto *data = static_cast<PrintPayload *>(p);
            std::cout << data->message << std::endl;
        })
        .build();
}

} // namespace star::job::tasks::task_factory