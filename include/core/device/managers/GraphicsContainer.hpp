#pragma once

#include "Shader.hpp"
#include "Pipeline.hpp"

namespace star::core::device::manager
{
struct GraphicsContainer
{
    Shader shaderManager; 
    Pipeline pipelineManager; 
};
} // namespace star::core::device::managers