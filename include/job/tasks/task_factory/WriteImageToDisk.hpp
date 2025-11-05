#pragma once

#include "job/complete_tasks/CompleteTask.hpp"
#include "job/tasks/Task.hpp"

#include "StarTextures/Texture.hpp"

#include <optional>

namespace star::job::tasks::task_factory::write_image_to_disk
{

struct WritePayload
{
    std::unique_ptr<star::StarTextures::Texture> texture = nullptr;
    std::string path = std::string(); 
};

using WriteImageTask = star::job::tasks::Task<sizeof(WritePayload), alignof(WritePayload)>;

std::optional<star::job::complete_tasks::CompleteTask<>> CreateComplete(void *p);

void Execute(void *p);

WriteImageTask Create(std::unique_ptr<StarTextures::Texture> texture, std::string filePath);
} // namespace star::job::task_factory::write_image_to_disk