#pragma once

#include "StarPipeline.hpp"

#include <vector>

namespace star::core::renderer
{
/// @brief Contains information needed for objects to process their rendering commands
struct RenderingContext
{
    StarPipeline* pipeline = nullptr; 
};
} // namespace star::core::renderer