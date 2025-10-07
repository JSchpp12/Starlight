#pragma once

#include "Shader.hpp"
#include "Pipeline.hpp"
#include "Semaphore.hpp"

namespace star::core::device::manager
{
struct GraphicsContainer
{
    Shader shaderManager; 
    Pipeline pipelineManager; 
    Semaphore semaphoreManager;
};
} // namespace star::core::device::managers