#pragma once

#include "StarPipeline.hpp"

#include <vector>

namespace star::core::renderer
{
/// @brief Contains information needed for objects to process their rendering commands
struct RenderingContext
{
    RenderingContext(star::StarPipeline &pipeline) : pipeline(pipeline){}
    StarPipeline& pipeline; 
};
} // namespace star::core::renderer