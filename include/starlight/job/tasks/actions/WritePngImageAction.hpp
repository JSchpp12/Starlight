#pragma once

#include "StarBuffers/Buffer.hpp"

#include <string>
#include <vulkan/vulkan.hpp>

namespace star::job::tasks::actions
{

struct WritePngImageAction
{
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    std::string path;
    const StarBuffers::Buffer &buffer;

    void operator()();
};

bool IsPngFormat(vk::Format fmt);

} // namespace star::job::tasks::actions