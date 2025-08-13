#include "TaskFactory.hpp"

void star::job::TaskFactory::printExecute(void *p)
{
    auto *payload = static_cast<PrintPayload *>(p);
    std::cout << "Print task: " << payload->message << std::endl;
}

star::job::Task<> star::job::TaskFactory::createPrintTask(std::string message)
{
    return star::job::Task<>::Builder<PrintPayload>()
        .setPayload(PrintPayload(std::move(message)))
        .setExecute(&TaskFactory::printExecute)
        .build();
}
