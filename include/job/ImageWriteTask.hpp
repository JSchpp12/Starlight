#pragma once

#include "StarTextures/Texture.hpp"
#include "Task.hpp"

#include <string>

namespace star::Job
{
class ImageWriteTask : public Task<StarTextures::Texture>
{
  public:
    ImageWriteTask(StarTextures::Texture texture, const std::string &fileName)
        : Task<StarTextures::Texture>(texture), fileName(fileName)
    {
    }

    virtual ~ImageWriteTask() = default;

  private:
    std::string fileName = "";
};
} // namespace star::Job