#pragma once

#include "Enums.hpp"
#include "StarCommandPool.hpp"
#include "StarQueue.hpp"

#include "vulkan/vulkan.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace star
{
/// <summary>
/// Create reusable command buffer object
/// </summary>
class StarCommandBuffer
{
  public:
    StarCommandBuffer(const StarCommandBuffer &) = delete;
    StarCommandBuffer &operator=(const StarCommandBuffer &) = delete;
    StarCommandBuffer(StarCommandBuffer &&) = default;
    StarCommandBuffer &operator=(StarCommandBuffer &&) = default;

    StarCommandBuffer(vk::Device &device, int numBuffersToCreate, std::shared_ptr<StarCommandPool> parentPool,
                      const Queue_Type type, bool initFences, bool initSemaphores);

    ~StarCommandBuffer();

    void cleanupRender(vk::Device &device);

    /// <summary>
    /// Signal for begin of command recording.
    /// </summary>
    /// <param name="buffIndex">Buffer index to prepare.(Should equal swap chain image number in main renderer)</param>
    /// <returns>The command buffer which is ready for command recording.</returns>
    void begin(int buffIndex = 0);

    /// <summary>
    /// Signal for begin of command recording. This function will allow callee to manually define begin information.
    /// </summary>
    /// <param name="buffIndex">Buffer index to prepare.(Should equal swap chain image number in main renderer)</param>
    /// <param name="beginInfo">Vulkan begin info</param>
    /// <returns>The command buffer which is ready for command recording.</returns>
    void begin(const int buffIndex, const vk::CommandBufferBeginInfo &beginInfo);

    /// <summary>
    /// This command buffer should wait for the provided semaphores to complete before proceeding.
    /// This wait information will be provided to vulkan during submit().
    /// </summary>
    /// <param name="semaphores"></param>
    /// <param name="flags"></param>
    void waitFor(std::vector<vk::Semaphore> semaphores, vk::PipelineStageFlags whereWait);

    /// <summary>
    /// Wait for another buffer to complete
    /// </summary>
    /// <param name="otherBuffer"></param>
    /// <param name="whereWait"></param>
    void waitFor(StarCommandBuffer &otherBuffer, vk::PipelineStageFlags whereWait);

    void reset(int bufferIndex);

    void submit(int bufferIndex, vk::Queue &targetQueue,
                std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags>> *additionalWaits = nullptr,
                std::vector<std::optional<uint64_t>> *additionalWaitsSignaledValues = nullptr,
                vk::Fence *additionalFences = nullptr,
                std::vector<vk::Semaphore> *additionalSignalSemaphores = nullptr);

    bool isFenceReady(const int &bufferIndex);

    /// <summary>
    /// Returns the semaphores that will be signaled once this buffer is done executing.
    /// </summary>
    std::vector<vk::Semaphore> &getCompleteSemaphores();

    vk::CommandBuffer &buffer(const size_t &buffIndex = 0)
    {
        return this->commandBuffers.at(buffIndex);
    }

    vk::Fence &getFence(const size_t &bufferIndex = 0)
    {
        return this->readyFence.at(bufferIndex);
    }

    void wait(int bufferIndex = 0);

    Queue_Type getType()
    {
        return this->type;
    }

    size_t getNumBuffers()
    {
        return this->commandBuffers.size();
    }

  protected:
    vk::Device vulkanDevice;
    Queue_Type type;
    std::shared_ptr<StarCommandPool> parentPool = nullptr;
    StarCommandBuffer *mustWaitFor = nullptr;

    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Semaphore> completeSemaphores;
    std::vector<vk::Fence> readyFence;
    std::vector<std::vector<std::pair<vk::Semaphore, vk::PipelineStageFlags>>> waitSemaphores;

    bool recorded = false;

    void createSemaphores();

    void createTracking();

    void createFences();

    static std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::Device &vulkanDevice, vk::CommandPool &commandPool,
                                                               const uint32_t &numToCreate);
};
} // namespace star