#pragma once

#include "Pipeline.hpp"
#include "Semaphore.hpp"
#include "Shader.hpp"


namespace star::core::device::manager
{
struct GraphicsContainer
{
    GraphicsContainer() = default;
    GraphicsContainer(const GraphicsContainer &) noexcept = delete;
    GraphicsContainer &operator=(const GraphicsContainer &) noexcept = delete;
    GraphicsContainer(GraphicsContainer &&) noexcept = default;
    GraphicsContainer &operator=(GraphicsContainer &&) noexcept = default;
    ~GraphicsContainer() = default;

    Shader shaderManager;
    Pipeline pipelineManager;
    Semaphore semaphoreManager;
};
} // namespace star::core::device::manager