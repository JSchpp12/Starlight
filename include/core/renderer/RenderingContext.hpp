#pragma once

#include "StarPipeline.hpp"
#include "MappedHandleContainer.hpp"

#include <vector>

namespace star::core::renderer
{
/// @brief Contains information needed for objects to process their rendering commands
struct RenderingContext
{
    StarPipeline* pipeline = nullptr;
    MappedHandleContainer<vk::Buffer, star::Handle_Type::buffer> bufferTransferRecords = MappedHandleContainer<vk::Buffer, star::Handle_Type::buffer>();  
};
} // namespace star::core::renderer