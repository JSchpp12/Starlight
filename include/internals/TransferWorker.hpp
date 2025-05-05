#pragma once

#include "Handle.hpp"
#include "SharedFence.hpp"
#include "Allocator.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"

#include "SharedFence.hpp"
#include "StarQueueFamily.hpp"
#include "StarTexture.hpp"

#include <vulkan/vulkan.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/stack.hpp>

#include <vector>
#include <memory>
#include <queue>
#include <optional>

namespace star{
    class TransferManagerThread {
        public:
        struct InterThreadRequest{
            std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest = nullptr;
            std::unique_ptr<TransferRequest::Texture> textureTransferRequest = nullptr;
            std::optional<std::unique_ptr<StarBuffer>*> resultingBuffer = std::nullopt; 
            std::optional<std::unique_ptr<StarTexture>*> resultingTexture = std::nullopt;
            SharedFence* completeFence = nullptr;
            boost::atomic<bool>* cpuWorkDoneByTransferThread = nullptr;

            InterThreadRequest(boost::atomic<bool>* cpuWorkDoneByTransferThread, SharedFence* completeFence, std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest, std::unique_ptr<StarBuffer>& resultingBufferAddress) 
                : bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBufferAddress), completeFence(completeFence), cpuWorkDoneByTransferThread(cpuWorkDoneByTransferThread){}

            InterThreadRequest(boost::atomic<bool>* cpuWorkDoneByTransferThread, SharedFence* completeFence, std::unique_ptr<TransferRequest::Texture> textureTransferRequest, std::unique_ptr<StarTexture>& resultingTextureAddress)
                : cpuWorkDoneByTransferThread(cpuWorkDoneByTransferThread), completeFence(completeFence), resultingTexture(&resultingTextureAddress), textureTransferRequest(std::move(textureTransferRequest)){}
        };

        struct InProcessRequestDependencies{
            std::optional<std::unique_ptr<StarBuffer>> transferSourceBuffer = std::nullopt; 
            SharedFence* completeFence; 

            InProcessRequestDependencies(std::unique_ptr<StarBuffer> transferSourceBuffer, SharedFence* completeFence) 
            : transferSourceBuffer(std::move(transferSourceBuffer)), completeFence(completeFence) {}
        };

        TransferManagerThread(StarDevice& device, Allocator& allocator, std::vector<boost::lockfree::stack<InterThreadRequest*>*> requestQueues, 
            const vk::PhysicalDeviceProperties& deviceLimits, std::unique_ptr<StarQueueFamily> ownedQueue);

        ~TransferManagerThread();

        void startAsync(); 

        void stopAsync();

        void cleanup();

        bool isFenceInUse(const SharedFence& fence, const bool& clearIfFound = true); 

        static void mainLoop(boost::atomic<bool>* shouldRun, vk::Device* device, 
            vk::CommandPool* transferCommandPool, vk::Queue* transferQueue, 
            VmaAllocator* allocator, const vk::PhysicalDeviceProperties* physicalProperties,
            std::vector<SharedFence*>* commandBufferFences, 
            std::vector<boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest*>*>* workingRequestQueues);

        static std::vector<vk::CommandBuffer> createCommandBuffers(vk::Device& device, vk::CommandPool pool, 
            const uint8_t& numToCreate);

        static void createBuffer(vk::Device& device, VmaAllocator& allocator, 
            vk::Queue& transferQueue, const vk::PhysicalDeviceProperties& deviceProperties, SharedFence& workCompleteFence, 
            std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, const size_t& bufferIndexToUse, 
            std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences,
            TransferRequest::Buffer* newBufferRequest, std::unique_ptr<StarBuffer>* resultingBuffer);

        static void createTexture(vk::Device& device, VmaAllocator& allocator,
            vk::Queue& transferQueue, const vk::PhysicalDeviceProperties& deviceProperties, SharedFence& workCompleteFence, 
            std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, const size_t& bufferIndexToUse, 
            std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences,
            TransferRequest::Texture* newTextureRequest, std::unique_ptr<StarTexture>* resultingTexture);

        static void checkForCleanups(vk::Device& device, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, std::vector<SharedFence*>& commandBufferFences); 

        static void readyCommandBuffer(vk::Device& device, const size_t& indexSelected, std::vector<SharedFence*>& commandBufferFences); 

        protected:
        bool ownsVulkanResources = false;
        std::queue<std::unique_ptr<InProcessRequestDependencies>> inProcessRequests = std::queue<std::unique_ptr<InProcessRequestDependencies>>();
        
        StarDevice& device;
        std::unique_ptr<StarQueueFamily> transferQueue = nullptr; 
        const vk::PhysicalDeviceProperties deviceProperties;
        std::vector<vk::CommandBuffer> commandBuffers;
        std::vector<SharedFence*> commandBufferFences;
        size_t previousBufferIndexUsed = 0; 
        Allocator& allocator; 

        std::vector<boost::lockfree::stack<InterThreadRequest*>*> requestQueues; 

        boost::atomic<bool> shouldRun = false;
        boost::thread thread;

        static std::vector<vk::Queue> createTransferQueues(star::StarDevice& device, const uint32_t& dedicatedTransferQueueFamilyIndex); 

        static void transitionImageLayout(vk::Image& image, vk::CommandBuffer& commandBuffer, const vk::Format& format, const vk::ImageLayout& oldLayout, const vk::ImageLayout& newLayout);
    };

    class TransferWorker {
    public:
    /// @brief Creates a transfer worker which does not own the dedicated transfer queue. Device might not support dedicated transfer queue. As such, transfers are submitted on the main thread.
    /// @param device Created star device from which vulkan objects can be made
    TransferWorker(StarDevice& device, bool overrideRunAsync);

    void add(SharedFence& workCompleteFence, boost::atomic<bool>& isBeingWorkedOnByTransferThread, 
        std::unique_ptr<TransferRequest::Buffer> newBufferRequest, 
        std::unique_ptr<StarBuffer>& resultingBuffer, const bool& isHighPriority);

    void add(SharedFence& workCompleteFence, boost::atomic<bool>& isBeingWorkedOnByTransferThread, 
        std::unique_ptr<TransferRequest::Texture> newTextureRequest, 
        std::unique_ptr<StarTexture>& resultingTexture, const bool& isHighPriority);

    void update(); 
    
    ~TransferWorker(); 

    private:
    StarDevice& device; 

boost::lockfree::stack<TransferManagerThread::InterThreadRequest*> highPriorityRequests = boost::lockfree::stack<TransferManagerThread::InterThreadRequest*>(50);
    boost::lockfree::stack<TransferManagerThread::InterThreadRequest*> standardRequests = boost::lockfree::stack<TransferManagerThread::InterThreadRequest*>(50); 
    std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>> requests = std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>>(); 
    std::vector<std::unique_ptr<TransferManagerThread>> threads = std::vector<std::unique_ptr<TransferManagerThread>>();

    void checkFenceStatus(TransferManagerThread::InterThreadRequest& newRequest); 

    void insertRequest(std::unique_ptr<TransferManagerThread::InterThreadRequest> newRequest, const bool& isHighPriority);

    void checkForCleanups(); 

    };
}
