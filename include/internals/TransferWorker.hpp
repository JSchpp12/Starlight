#pragma once

#include "Allocator.hpp"
#include "Handle.hpp"
#include "StarBuffers/Buffer.hpp"
#include "devices/StarDevice.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"

#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "StarQueueFamily.hpp"
#include "StarTextures/Texture.hpp"

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
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;
        std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest = nullptr;
        std::unique_ptr<TransferRequest::Texture> textureTransferRequest = nullptr;
        std::optional<std::unique_ptr<StarBuffers::Buffer> *> resultingBuffer = std::nullopt;
        std::optional<std::unique_ptr<StarTextures::Texture> *> resultingTexture = std::nullopt;

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer)
            : gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBuffer)
        {
        }

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Texture> textureTransferRequest,
                           std::unique_ptr<StarTextures::Texture> &resultingTexture)
            : gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              textureTransferRequest(std::move(textureTransferRequest)), resultingTexture(&resultingTexture)
        {
        }
    };

    class ProcessRequestInfo
    {
      public:
        std::unique_ptr<StarCommandBuffer> commandBuffer = nullptr;

        ProcessRequestInfo(std::unique_ptr<StarCommandBuffer> commandBuffer)
            : commandBuffer(std::move(commandBuffer)) {};

        void setInProcessDeps(std::unique_ptr<StarBuffers::Buffer> nInProcessTransferSrcBuffer,
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
        std::unique_ptr<StarBuffers::Buffer> inProcessTransferSrcBuffer = nullptr;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;
    };

    struct SubThreadInfo
    {
        SubThreadInfo(boost::atomic<bool> *shouldRun, vk::Device &device,
                      StarQueue queue, VmaAllocator &allocator,
                      vk::PhysicalDeviceProperties deviceProperties,
                      std::vector<boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest *> *>
                          *workingRequestQueues,
                      std::vector<uint32_t> allTransferQueueFamilyIndicesInUse)
            : shouldRun(shouldRun), device(device), queue(queue), allocator(allocator),
              deviceProperties(deviceProperties), workingRequestQueues(workingRequestQueues),
              allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse)
        {
        }

        boost::atomic<bool> *shouldRun = nullptr;
        vk::Device &device;
        StarQueue queue; 
        VmaAllocator &allocator;
        vk::PhysicalDeviceProperties deviceProperties = vk::PhysicalDeviceProperties();
        std::vector<boost::lockfree::stack<InterThreadRequest *> *> *workingRequestQueues = nullptr;
        std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>();
    };

    TransferManagerThread(core::devices::StarDevice &device, std::vector<boost::lockfree::stack<InterThreadRequest *> *> requestQueues,
                          const vk::PhysicalDeviceProperties &deviceProperties, StarQueue myQueue,
                          const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse);

    ~TransferManagerThread();

    //no copy
    TransferManagerThread(const TransferManagerThread &) = delete;
      TransferManagerThread &operator=(TransferManagerThread &&) = delete;
    //no move
    TransferManagerThread(TransferManagerThread &&) = delete;

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
                             std::unique_ptr<StarBuffers::Buffer> *resultingBuffer, boost::atomic<bool> *gpuDoneSignalToMain);

    static void CreateTexture(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                              const vk::PhysicalDeviceProperties &deviceProperties,
                              const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                              ProcessRequestInfo &processInfo, TransferRequest::Texture *newTextureRequest,
                              std::unique_ptr<StarTextures::Texture> *resultingTexture, boost::atomic<bool> *gpuDoneSignalToMain);

    static void CheckForCleanups(vk::Device &device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos);

    static void EnsureInfoReady(vk::Device &device, ProcessRequestInfo &processInfo);

  protected:
    core::devices::StarDevice &device;
    std::vector<boost::lockfree::stack<InterThreadRequest *> *> requestQueues;
    vk::PhysicalDeviceProperties deviceProperties;
    StarQueue myQueue; 
    std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>();

    boost::atomic<bool> shouldRun = false;
    boost::thread thread;
    std::shared_ptr<StarCommandPool> myCommandPool = nullptr;
};

class TransferWorker
{
  public:
    /// @brief Creates a transfer worker which does not own the dedicated transfer queue. Device might not support
    /// dedicated transfer queue. As such, transfers are submitted on the main thread.
    /// @param device Created star device from which vulkan objects can be made
    TransferWorker(core::devices::StarDevice &device, bool overrideRunAsync, std::vector<StarQueue> &queuesToUse);

    void add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
             std::unique_ptr<TransferRequest::Buffer> newBufferRequest, std::unique_ptr<StarBuffers::Buffer> &resultingBuffer,
             const bool &isHighPriority);

    void add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
             std::unique_ptr<TransferRequest::Texture> newTextureRequest,
             std::unique_ptr<StarTextures::Texture> &resultingTexture, const bool &isHighPriority);

    void update();

    ~TransferWorker();

  private:
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
        core::devices::StarDevice &device, const std::vector<StarQueue> queuesToUse,
        boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &highPriorityQueue,
        boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &standardQueue);
};
} // namespace star
