#include "TaskFactory.hpp"

void star::job::TaskFactory::printExecute(void *p)
{
    auto *payload = static_cast<PrintPayload *>(p);
    std::cout << "Print task: " << payload->message << std::endl;
}

void star::job::TaskFactory::imageWriteExecute(void *p)
{

}

star::job::Task<> star::job::TaskFactory::createPrintTask(std::string message)
{
    return star::job::Task<>::Builder<PrintPayload>()
        .setPayload(PrintPayload(std::move(message)))
        .setExecute(&TaskFactory::printExecute)
        .build();
}

star::job::Task<> star::job::TaskFactory::createImageWriteTask(std::string fileName, StarBuffers::Buffer imageBuffer)
{
    auto resources = imageBuffer.releaseResources(); 

    auto task = star::job::Task<>::Builder<ImageWritePayload>()
        .setPayload(ImageWritePayload(fileName.c_str(), resources->allocator, resources->memory, imageBuffer.getBufferSize(), resources->buffer))
        .setExecute(&TaskFactory::imageWriteExecute)
        .build(); 

    resources->buffer = VK_NULL_HANDLE;
    return task; 
}
