#include "job/tasks/WriteImageToDisk.hpp"

#include "FileHelpers.hpp"

namespace star::job::tasks::write_image_to_disk
{

std::optional<star::job::complete_tasks::CompleteTask> CreateComplete(void *p)
{
    return std::nullopt;
}

void Execute(void *p)
{
    auto *payload = static_cast<WritePayload *>(p);

    auto parentDir = file_helpers::GetParentDirectory(payload->path);
    if (!boost::filesystem::exists(parentDir))
    {
        file_helpers::CreateDirectoryIfDoesNotExist(parentDir);
    }
}

WriteImageTask Create(std::unique_ptr<StarTextures::Texture> texture, std::string filePath)
{
    return WriteImageTask::Builder<WritePayload>()
        .setPayload(WritePayload{.texture = std::move(texture), .path = std::move(filePath)})
        .setCreateCompleteTaskFunction(&CreateComplete)
        .setExecute(&Execute)
        .build();
}

} // namespace star::job::tasks::write_image_to_disk