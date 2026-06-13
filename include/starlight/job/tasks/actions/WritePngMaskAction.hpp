#pragma once

#include "job/tasks/actions/ImageDataTypes.hpp"

#include <string>
#include <vulkan/vulkan.hpp>

namespace star::job::tasks::actions
{

struct WritePngMaskAction
{
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    std::string path;
    ImageDataSource dataSource;

    void operator()();
};

bool IsPngMaskFormat(vk::Format fmt);

} // namespace star::job::tasks::actions