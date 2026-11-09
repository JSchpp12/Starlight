#pragma once

#include "Buffer.hpp"
#include "DescriptorPool.hpp"
#include "Fence.hpp"
#include "Image.hpp"
#include "Material.hpp"
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
        : queueManager(std::move(other.queueManager)), bufferManager(std::move(other.bufferManager)),
          imageManager(std::move(other.imageManager)), materialManager(std::move(other.materialManager)),
          descriptorPoolManager(std::move(other.descriptorPoolManager)),
          semaphoreManager(std::move(other.semaphoreManager)), shaderManager(std::move(other.shaderManager)),
          pipelineManager(std::move(other.pipelineManager)), fenceManager(std::move(other.fenceManager)) {};
    GraphicsContainer &operator=(GraphicsContainer &&other) noexcept
    {
        if (this != &other)
        {
            queueManager = std::move(other.queueManager);
            bufferManager = std::move(other.bufferManager);
            descriptorPoolManager = std::move(other.descriptorPoolManager);
            semaphoreManager = std::move(other.semaphoreManager);
            shaderManager = std::move(other.shaderManager);
            pipelineManager = std::move(other.pipelineManager);
            fenceManager = std::move(other.fenceManager);
            imageManager = std::move(other.imageManager);
            materialManager = std::move(other.materialManager);
        }
        return *this;
    };
    ~GraphicsContainer() = default;

    void init(StarDevice *device, common::EventBus &bus, job::TaskManager &taskSystem, const uint8_t &numFramesInFlight)
    {
        queueManager.init(device);
        bufferManager.init(device, bus);
        imageManager.init(device, bus);
        materialManager.init(device, bus);
        descriptorPoolManager->init(numFramesInFlight, device, bus);
        semaphoreManager->init(device, bus);
        shaderManager->init(device, bus, taskSystem);
        pipelineManager->init(device, bus, taskSystem);
        fenceManager->init(device, bus);
    }

    void cleanupRender()
    {
        queueManager.cleanupRender();
        bufferManager.cleanupRender();
        imageManager.cleanupRender();
        materialManager.cleanupRender();
        descriptorPoolManager->cleanupRender();
        fenceManager->cleanupRender();
        pipelineManager->cleanupRender();
        shaderManager->cleanupRender();
        semaphoreManager->cleanupRender();
    }

    Queue queueManager;
    Buffer bufferManager;
    Image imageManager;
    Material materialManager;
    std::unique_ptr<DescriptorPool> descriptorPoolManager = std::make_unique<DescriptorPool>();
    std::unique_ptr<Semaphore> semaphoreManager = std::make_unique<Semaphore>();
    std::unique_ptr<Shader> shaderManager = std::make_unique<Shader>();
    std::unique_ptr<Pipeline> pipelineManager = std::make_unique<Pipeline>();
    std::unique_ptr<Fence> fenceManager = std::make_unique<Fence>();
};
} // namespace star::core::device::manager