#pragma once

#include "Handle.hpp"
#include "SharedFence.hpp"
#include "Allocator.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "BufferMemoryTransferRequest.hpp"
#include "SharedFence.hpp"

#include <vulkan/vulkan.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/stack.hpp>

#include <vector>
#include <memory>
#include <thread>
#include <map>
#include <optional>
#include <queue>
#include <thread>

namespace star{
    class TransferManagerThread {
        public:
        struct InterThreadRequest{
            std::unique_ptr<BufferMemoryTransferRequest> bufferTransferRequest = nullptr;
            std::optional<std::unique_ptr<StarBuffer>*> resultingBuffer = std::nullopt; 
            SharedFence* completeFence = nullptr;
            boost::atomic<bool>* cpuWorkDoneByTransferThread = nullptr;

            InterThreadRequest(std::unique_ptr<BufferMemoryTransferRequest> request, std::unique_ptr<StarBuffer>& resultingBufferAddress, 
                SharedFence* completeFence, boost::atomic<bool>* cpuWorkDoneByTransferThread) 
                : bufferTransferRequest(std::move(request)), resultingBuffer(&resultingBufferAddress), completeFence(completeFence), cpuWorkDoneByTransferThread(cpuWorkDoneByTransferThread){}
        };

        struct InProcessRequestDependencies{
            std::optional<std::unique_ptr<StarBuffer>> transferSourceBuffer = std::nullopt; 
            SharedFence* completeFence; 

            InProcessRequestDependencies(std::unique_ptr<StarBuffer> transferSourceBuffer, SharedFence* completeFence) 
            : transferSourceBuffer(std::move(transferSourceBuffer)), completeFence(completeFence) {}
        };

        TransferManagerThread(StarDevice& device, Allocator& allocator, 
            vk::PhysicalDeviceLimits deviceLimits, vk::Queue transferQueue, vk::CommandPool sharedCommandPool);

        ~TransferManagerThread();

        void startAsync(); 

        void stopAsync();

        void add(std::unique_ptr<InterThreadRequest> request, const bool& isHighPriority = false);

        void cleanup();

        static void mainLoop(boost::atomic<bool>* shouldRun, vk::Device* device, 
            vk::CommandPool* transferCommandPool, vk::Queue* transferQueue, 
            VmaAllocator* allocator, vk::PhysicalDeviceLimits* limits,
            std::vector<SharedFence*>* commandBufferFences, 
            boost::lockfree::stack<InterThreadRequest*>* highPriorityRequests, 
            boost::lockfree::stack<InterThreadRequest*>* standardTransferRequests);

        static std::vector<vk::CommandBuffer> createCommandBuffers(vk::Device& device, vk::CommandPool pool, 
            const uint8_t& numToCreate);

        static void updateBufferData(vk::Device& device);

        static void createBuffer(vk::Device& device, VmaAllocator& allocator, 
            vk::Queue& transferQueue, vk::PhysicalDeviceLimits& limits, SharedFence& workCompleteFence, 
            BufferMemoryTransferRequest* newBufferRequest, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, 
            size_t bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences,
            std::unique_ptr<StarBuffer>* resultingBuffer);

        static void checkForCleanups(vk::Device& device, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, std::vector<SharedFence*>& commandBufferFences); 

        static void readyCommandBuffer(vk::Device& device, const size_t& indexSelected, std::vector<SharedFence*>& commandBufferFences); 

        protected:
        std::optional<boost::lockfree::stack<InterThreadRequest*>> highPriorityRequests = std::nullopt;
        std::optional<boost::lockfree::stack<InterThreadRequest*>> standardRequests = std::nullopt; 

        bool ownsVulkanResources = false;
        std::queue<std::unique_ptr<InProcessRequestDependencies>> inProcessRequests = std::queue<std::unique_ptr<InProcessRequestDependencies>>();
        
        StarDevice& device;
        vk::CommandPool transferPool; 
        vk::PhysicalDeviceLimits deviceLimits;
        std::vector<vk::CommandBuffer> commandBuffers;
        std::vector<SharedFence*> commandBufferFences;
        size_t previousBufferIndexUsed = 0; 
        vk::Queue transferQueue;
        Allocator& allocator; 
        std::vector<std::unique_ptr<InterThreadRequest>> transferRequests = std::vector<std::unique_ptr<InterThreadRequest>>(); 

        boost::atomic<bool> shouldRun = false;
        boost::thread thread;

        static std::vector<vk::Queue> createTransferQueues(star::StarDevice& device, const uint32_t& dedicatedTransferQueueFamilyIndex); 

        void initMultithreadedDeps(); 
    };

    class TransferWorker {
    public:
    /// @brief Creates a transfer worker which does not own the dedicated transfer queue. Device might not support dedicated transfer queue. As such, transfers are submitted on the main thread.
    /// @param device Created star device from which vulkan objects can be made
    TransferWorker(StarDevice& device, star::Allocator& allocator,
        vk::Queue sharedTransferQueue, vk::CommandPool sharedCommandPool, const bool& runAsync);

    void add(std::unique_ptr<BufferMemoryTransferRequest> newBufferRequest, SharedFence& workCompleteFence,
        std::unique_ptr<StarBuffer>& resultingBuffer, boost::atomic<bool>& isBeingWorkedOnByTransferThread, const bool& isHighPriority);

    void update(); 
    
    ~TransferWorker(); 

    private:
    std::unique_ptr<TransferManagerThread> manager;
    };

}
