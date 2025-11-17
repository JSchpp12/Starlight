#pragma once

#include "Pipeline.hpp"
#include "Semaphore.hpp"
#include "Shader.hpp"
#include "Fence.hpp"
#include "core/device/system/EventBus.hpp"

#include <memory>

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

    void init(std::shared_ptr<StarDevice> device, core::device::system::EventBus &bus)
    {
        semaphoreManager->init(device, bus);
        shaderManager->init(device, bus);
        pipelineManager->init(device, bus);
        fenceManager->init(device, bus);
    }

    void cleanup(core::device::system::EventBus &bus){
        fenceManager->cleanup(bus);
        pipelineManager->cleanup(bus); 
        shaderManager->cleanup(bus);
        semaphoreManager->cleanup(bus);
    }

    std::shared_ptr<Semaphore> semaphoreManager = std::make_shared<Semaphore>();
    std::shared_ptr<Shader> shaderManager = std::make_shared<Shader>();
    std::shared_ptr<Pipeline> pipelineManager = std::make_shared<Pipeline>();
    std::shared_ptr<Fence> fenceManager = std::make_shared<Fence>();
};
} // namespace star::core::device::manager