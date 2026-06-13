#pragma once

#include "StarBuffers/Buffer.hpp"

#include <cstdint>
#include <variant>

namespace star::job::tasks::actions
{

struct VulkanBufferSource
{
    const StarBuffers::Buffer &buffer;
};

struct RawFloatSource
{
    const float *data;
};

struct RawUint8Source
{
    const uint8_t *data;
};

using ImageDataSource = std::variant<VulkanBufferSource, RawFloatSource, RawUint8Source>;

} // namespace star::job::tasks::actions