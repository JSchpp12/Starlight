#pragma once

#include "Pipeline.hpp"
#include "Semaphore.hpp"
#include "Shader.hpp"
#include "core/device/system/EventBus.hpp"

#include <memory>

namespace star::core::device::manager
{
struct GraphicsContainer
{
    GraphicsContainer() : semaphoreManager(std::make_shared<Semaphore>()){};
    GraphicsContainer(const GraphicsContainer &) noexcept = delete;
    GraphicsContainer &operator=(const GraphicsContainer &) noexcept = delete;
    GraphicsContainer(GraphicsContainer &&) noexcept = default;
    GraphicsContainer &operator=(GraphicsContainer &&) noexcept = default;
    ~GraphicsContainer() = default;

    void init(std::shared_ptr<StarDevice> device, core::device::system::EventBus &bus){
        semaphoreManager->init(device, bus);
    }

    std::shared_ptr<Semaphore> semaphoreManager = std::make_shared<Semaphore>();
    Shader shaderManager;
    Pipeline pipelineManager;
};
} // namespace star::core::device::manager