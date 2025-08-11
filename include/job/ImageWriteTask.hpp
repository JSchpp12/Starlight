#pragma once

#include "StarBuffers/Buffer.hpp"

#include "Task.hpp"
#include "stb_image.h"

#include <string>

namespace star::Job
{
class ImageWriteTask : public Task<StarBuffers::Buffer>
{
  public:
    ImageWriteTask(StarBuffers::Buffer buffer, const std::string &fileName)
        : Task<StarBuffers::Buffer>(buffer), fileName(fileName)
    {
    }

    virtual ~ImageWriteTask() = default;

  protected:
    virtual void run(StarBuffers::Buffer &texture) override{

    }

  private:
    std::string fileName = "";

};
} // namespace star::Job