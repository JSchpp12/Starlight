#pragma once

#include "job/tasks/actions/ImageDataTypes.hpp"

#include <string>
#include <vulkan/vulkan.hpp>

namespace star::job::tasks::actions
{

struct WriteTiffImageAction
{
    enum class Compression
    {
        none,
        zstd,
        lzw
    };
    enum class Precision
    {
        Float32,
        Uint16,
        Uint8
    };
    vk::Extent3D imageExtent;
    vk::Format imageFormat;
    std::string path;
    ImageDataSource dataSource;
    Compression compressionOption{Compression::none};
    Precision precision{Precision::Float32};

    void operator()();
};

bool IsTiffFormat(vk::Format fmt);

} // namespace star::job::tasks::actions