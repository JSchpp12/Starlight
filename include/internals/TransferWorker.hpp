#pragma once

#include "Allocator.hpp"
#include "Handle.hpp"
#include "SharedFence.hpp"
#include "StarBuffer.hpp"
#include "StarDevice.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"

#include "SharedFence.hpp"
#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "StarQueueFamily.hpp"
#include "StarTexture.hpp"

#include <boost/atomic.hpp>
#include <boost/lockfree/stack.hpp>
#include <boost/thread/thread.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace star
{
class TransferManagerThread
{
  public:
    struct InterThreadRequest
    {
        std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest = nullptr;
        std::unique_ptr<TransferRequest::Texture> textureTransferRequest = nullptr;
        std::optional<std::unique_ptr<StarBuffer> *> resultingBuffer = std::nullopt;
        std::optional<std::unique_ptr<StarTexture> *> resultingTexture = std::nullopt;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffer> &resultingBufferAddress)
            : bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBufferAddress),
              gpuDoneNotificationToMain(gpuDoneNotificationToMain)
        {
        }

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Texture> textureTransferRequest,
                           std::unique_ptr<StarTexture> &resultingTextureAddress)
            : gpuDoneNotificationToMain(gpuDoneNotificationToMain), resultingTexture(&resultingTextureAddress),
              textureTransferRequest(std::move(textureTransferRequest))
        {
        }
    };

    class ProcessRequestInfo
    {
      public:
        std::unique_ptr<StarCommandBuffer> commandBuffer = nullptr;

        ProcessRequestInfo(std::unique_ptr<StarCommandBuffer> commandBuffer)
            : commandBuffer(std::move(commandBuffer)) {};

        void setInProcessDeps(std::unique_ptr<StarBuffer> nInProcessTransferSrcBuffer,
                              boost::atomic<bool> *nGpuDoneNotificationToMain)
        {
            this->inProcessTransferSrcBuffer = std::move(nInProcessTransferSrcBuffer);
            this->gpuDoneNotificationToMain = nGpuDoneNotificationToMain;
        }

        void markAsAvailble()
        {
            this->inProcessTransferSrcBuffer.reset();

            this->gpuDoneNotificationToMain->store(true);
            this->gpuDoneNotificationToMain->notify_one();

            this->gpuDoneNotificationToMain = nullptr;
        }

        bool isMarkedAsAvailable() const
        {
            return this->gpuDoneNotificationToMain == nullptr;
        }

      private:
        std::unique_ptr<StarBuffer> inProcessTransferSrcBuffer = nullptr;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;
    };

    struct SubThreadInfo
    {
        SubThreadInfo(boost::atomic<bool> *shouldRun, vk::Device device, std::shared_ptr<StarCommandPool> commandPool,
                      std::vector<StarQueue> queues, VmaAllocator allocator,
                      vk::PhysicalDeviceProperties deviceProperties,
                      std::vector<boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest *> *>
                          *workingRequestQueues,
                      std::vector<uint32_t> allTransferQueueFamilyIndicesInUse)
            : shouldRun(shouldRun), device(device), commandPool(commandPool), queues(queues), allocator(allocator),
              deviceProperties(deviceProperties), workingRequestQueues(workingRequestQueues),
              allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse)
        {
        }

        boost::atomic<bool> *shouldRun = nullptr;
        std::vector<boost::lockfree::stack<InterThreadRequest *> *> *workingRequestQueues = nullptr;

        vk::Device device = vk::Device();
        VmaAllocator allocator = VmaAllocator();
        std::shared_ptr<StarCommandPool> commandPool = nullptr;
        std::vector<StarQueue> queues = std::vector<StarQueue>();
        vk::PhysicalDeviceProperties deviceProperties = vk::PhysicalDeviceProperties();
        std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>();
    };

    TransferManagerThread() = default;

    TransferManagerThread(StarDevice &device, VmaAllocator &allocator,
                          std::vector<boost::lockfree::stack<InterThreadRequest *> *> requestQueues,
                          const vk::PhysicalDeviceProperties &deviceLimits, std::vector<StarQueue> myQueues,
                          StarQueueFamily &queueFamilyToUse,
                          const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse);

    TransferManagerThread(const TransferManagerThread &) = delete;
    TransferManagerThread &operator=(const TransferManagerThread &) = delete;

    ~TransferManagerThread();

    void startAsync();

    void stopAsync();

    void cleanup();

    static void mainLoop(SubThreadInfo myInfo);

    static std::unique_ptr<ProcessRequestInfo> CreateProcessingInfo(vk::Device &device,
                                                                    std::shared_ptr<StarCommandPool> commandPool);

    static void CreateBuffer(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                             const vk::PhysicalDeviceProperties &deviceProperties,
                             const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                             ProcessRequestInfo &processInfo, TransferRequest::Buffer *newBufferRequest,
                             std::unique_ptr<StarBuffer> *resultingBuffer, boost::atomic<bool> *gpuDoneSignalToMain);

    static void CreateTexture(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                              const vk::PhysicalDeviceProperties &deviceProperties,
                              const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                              ProcessRequestInfo &processInfo, TransferRequest::Texture *newTextureRequest,
                              std::unique_ptr<StarTexture> *resultingTexture, boost::atomic<bool> *gpuDoneSignalToMain);

    static void CheckForCleanups(vk::Device &device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos);

    static void EnsureInfoReady(vk::Device &device, ProcessRequestInfo &processInfo);

  protected:
    StarQueueFamily &familyToUse;
    bool ownsVulkanResources = false;

    StarDevice &device;
    std::vector<StarQueue> myQueues;
    VmaAllocator allocator = VmaAllocator();
    std::shared_ptr<StarCommandPool> myCommandPool = nullptr;
    const vk::PhysicalDeviceProperties deviceProperties;
    size_t previousBufferIndexUsed = 0;
    std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>();

    std::vector<boost::lockfree::stack<InterThreadRequest *> *> requestQueues;

    boost::atomic<bool> shouldRun = false;
    boost::thread thread;

    static void transitionImageLayout(StarTexture &image, vk::CommandBuffer &commandBuffer, const vk::Format &format,
                                      const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout);
};

class TransferWorker
{
  public:
    /// @brief Creates a transfer worker which does not own the dedicated transfer queue. Device might not support
    /// dedicated transfer queue. As such, transfers are submitted on the main thread.
    /// @param device Created star device from which vulkan objects can be made
    TransferWorker(StarDevice &device, bool overrideRunAsync);

    void add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
             std::unique_ptr<TransferRequest::Buffer> newBufferRequest, std::unique_ptr<StarBuffer> &resultingBuffer,
             const bool &isHighPriority);

    void add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
             std::unique_ptr<TransferRequest::Texture> newTextureRequest,
             std::unique_ptr<StarTexture> &resultingTexture, const bool &isHighPriority);

    void update();

    ~TransferWorker();

  private:
    StarDevice &device;
    std::vector<std::unique_ptr<StarQueueFamily>> myQueueFamilies = std::vector<std::unique_ptr<StarQueueFamily>>();

    boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> highPriorityRequests =
        boost::lockfree::stack<TransferManagerThread::InterThreadRequest *>(50);
    boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> standardRequests =
        boost::lockfree::stack<TransferManagerThread::InterThreadRequest *>(50);
    std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>> requests =
        std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>>();

    std::vector<std::unique_ptr<TransferManagerThread>> threads = std::vector<std::unique_ptr<TransferManagerThread>>();

    void insertRequest(std::unique_ptr<TransferManagerThread::InterThreadRequest> newRequest,
                       const bool &isHighPriority);

    // void checkForCleanups();

    static std::vector<std::unique_ptr<TransferManagerThread>> CreateThreads(
        StarDevice &device, const std::vector<std::unique_ptr<StarQueueFamily>> &queueFamilies,
        boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &highPriorityQueue,
        boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &standardQueue);
};
} // namespace star
