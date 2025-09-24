#include "job/tasks/task_factory/WriteImageToDisk.hpp"

#include "FileHelpers.hpp"

namespace star::job::tasks::task_factory::write_image_to_disk
{

std::optional<star::job::complete_tasks::CompleteTask<>> CreateComplete(void *p)
{
    return std::nullopt;
}

void Execute(void *p)
{
    auto *payload = static_cast<WritePayload *>(p);

    auto parentDir = file_helpers::GetParentDirectory(payload->path);
    if (!boost::filesystem::exists(parentDir)){
        file_helpers::CreateDirectoryIfDoesNotExist(parentDir); 
    }
}

Task<> Create(std::unique_ptr<StarTextures::Texture> texture, std::string filePath)
{
    return Task<>::Builder<WritePayload>()
        .setPayload(WritePayload{.texture = std::move(texture), .path = std::move(filePath)})
        .setCreateCompleteTaskFunction(&CreateComplete)
        .setExecute(&Execute)
        .build();
}

} // namespace star::job::tasks::task_factory::write_image_to_disk