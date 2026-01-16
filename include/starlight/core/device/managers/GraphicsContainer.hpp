#pragma once

#include "CommandPool.hpp"
#include "DescriptorPool.hpp"
#include "Fence.hpp"
#include "Image.hpp"
#include "Pipeline.hpp"
#include "Queue.hpp"
#include "Semaphore.hpp"
#include "Shader.hpp"

#include <memory>

namespace star::core::device::manager
{
struct GraphicsContainer
{
    GraphicsContainer() = default;
    GraphicsContainer(const GraphicsContainer &) noexcept = delete;
    GraphicsContainer &operator=(const GraphicsContainer &) noexcept = delete;
    GraphicsContainer(GraphicsContainer &&other) noexcept
        : queueManager(std::move(other.queueManager)), commandPoolManager(std::move(other.commandPoolManager)),
          descriptorPoolManager(std::move(other.descriptorPoolManager)),
          semaphoreManager(std::move(other.semaphoreManager)), shaderManager(std::move(other.shaderManager)),
          pipelineManager(std::move(other.pipelineManager)), fenceManager(std::move(other.fenceManager)),
          imageManager(std::move(other.imageManager)) {};
    GraphicsContainer &operator=(GraphicsContainer &&other) noexcept
    {
        if (this != &other)
        {
            queueManager = std::move(other.queueManager);
            commandPoolManager = std::move(other.commandPoolManager);
            descriptorPoolManager = std::move(other.descriptorPoolManager);
            semaphoreManager = std::move(other.semaphoreManager);
            shaderManager = std::move(other.shaderManager);
            pipelineManager = std::move(other.pipelineManager);
            fenceManager = std::move(other.fenceManager);
            imageManager = std::move(other.imageManager);
        }
        return *this;
    };
    ~GraphicsContainer() = default;

    void init(StarDevice *device, common::EventBus &bus, job::TaskManager &taskSystem, const uint8_t &numFramesInFlight)
    {
        queueManager.init(device);
        commandPoolManager.init(device);
        descriptorPoolManager->init(numFramesInFlight, device, bus);
        semaphoreManager->init(device, bus);
        shaderManager->init(device, bus, taskSystem);
        pipelineManager->init(device, bus, taskSystem);
        fenceManager->init(device, bus);
        imageManager.init(device, bus);
    }

    void cleanupRender()
    {
        queueManager.cleanupRender();
        commandPoolManager.cleanupRender();
        descriptorPoolManager->cleanupRender();
        fenceManager->cleanupRender();
        pipelineManager->cleanupRender();
        shaderManager->cleanupRender();
        semaphoreManager->cleanupRender();
        imageManager.cleanupRender();
    }

    Queue queueManager;
    CommandPool commandPoolManager;
    std::unique_ptr<DescriptorPool> descriptorPoolManager = std::make_unique<DescriptorPool>();
    std::unique_ptr<Semaphore> semaphoreManager = std::make_unique<Semaphore>();
    std::unique_ptr<Shader> shaderManager = std::make_unique<Shader>();
    std::unique_ptr<Pipeline> pipelineManager = std::make_unique<Pipeline>();
    std::unique_ptr<Fence> fenceManager = std::make_unique<Fence>();
    Image imageManager;
};
} // namespace star::core::device::manager