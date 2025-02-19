#pragma once

#include "Handle.hpp"
#include "Allocator.hpp"
#include "StarDevice.hpp"
#include "StarBuffer.hpp"
#include "BufferManagerRequest.hpp"

#include <vulkan/vulkan.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/stack.hpp>

#include <tbb/tbb.h>

#include <vector>
#include <memory>
#include <thread>
#include <map>
#include <optional>
#include <queue>

namespace star{
    
    // class TransferThread{
    //     public:
    //         TransferThread(vk::Queue transferQueue, vk::Device vulkanDevice, vk::CommandBuffer myCommandBuffer, Allocator myAllocator); 
            
    //     private:
    //         // static void destroyObject();

    //         // static void mainLoop(vk::Queue transferQueue, vk::Device device, boost::atomic<bool>* shouldRunFlag);
    // };

    class TransferManagerThread {
        public:
        struct InterThreadRequest{
            std::optional<BufferManagerRequest*> bufferTransferRequest = std::nullopt;
            std::optional<std::unique_ptr<StarBuffer>*> resultingBuffer = std::nullopt; 
            vk::Fence* completeFence = nullptr;

            InterThreadRequest(BufferManagerRequest& request, std::unique_ptr<StarBuffer>& resultingBufferAddress, 
                vk::Fence* completeFence) 
                : bufferTransferRequest(&request), resultingBuffer(&resultingBufferAddress), completeFence(completeFence) {}

        };

        struct InProcessRequestDependencies{
            std::optional<std::unique_ptr<StarBuffer>> transferSourceBuffer = std::nullopt; 
            vk::Fence* completeFence; 

            InProcessRequestDependencies(std::unique_ptr<StarBuffer>  transferSourceBuffer, vk::Fence* completeFence) 
            : transferSourceBuffer(std::move(transferSourceBuffer)), completeFence(completeFence) {}
        };

        // TransferManagerThread(StarDevice& device, boost::lockfree::stack<BufferManagerRequest*, boost::lockfree::capacity<50>>& highPriorityRequests, 
        //     boost::lockfree::stack<BufferManagerRequest*, boost::lockfree::capacity<50>>& standardTransferRequests);

        TransferManagerThread(vk::Device& device, Allocator& allocator, 
            vk::PhysicalDeviceLimits deviceLimits);

        TransferManagerThread(vk::Device& device, Allocator& allocator, 
            vk::PhysicalDeviceLimits deviceLimits, vk::Queue sharedTransferQueue, vk::CommandPool sharedCommandPool);

        ~TransferManagerThread();

        void stop() { 
            this->shouldRun.store(false);
            
            //wait for thread to exit
            this->thread.join();
        }

        void add(InterThreadRequest request, const bool& isHighPriority = false);

        void inSyncCleanup();

        //Manually create a buffer object, should only be used in non-async mode
        std::unique_ptr<star::StarBuffer> syncCreate(BufferManagerRequest* newBufferRequest, boost::atomic<vk::Fence>& workCompleteFence);

        static void mainLoop(boost::atomic<bool>& shouldRun, boost::lockfree::stack<InterThreadRequest, boost::lockfree::capacity<50>>& highPriorityRequests, 
            boost::lockfree::stack<InterThreadRequest, boost::lockfree::capacity<50>>& standardTransferRequests);

        static std::vector<vk::CommandBuffer> createCommandBuffers(vk::Device& device, vk::CommandPool pool, 
            const uint8_t& numToCreate);

        static std::unique_ptr<StarBuffer> createBuffer(vk::Device& device, Allocator& allocator, 
            vk::Queue& transferQueue, vk::PhysicalDeviceLimits& limits, vk::Fence* workCompleteFence, 
            BufferManagerRequest *newBufferRequest, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, 
            size_t bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<vk::Fence*>& commandBufferFences);

        static void checkForCleanups(vk::Device& device, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, std::vector<vk::Fence*>& commandBufferFences); 

        static void readyCommandBuffer(vk::Device& device, const size_t& indexSelected, std::vector<vk::Fence*>& commandBufferFences); 

        protected:
        std::optional<boost::lockfree::stack<InterThreadRequest, boost::lockfree::capacity<50>>> highPriorityRequests = std::nullopt;
        std::optional<boost::lockfree::stack<InterThreadRequest, boost::lockfree::capacity<50>>> standardTransferRequests = std::nullopt; 

        bool ownsVulkanResources = false;
        std::queue<std::unique_ptr<InProcessRequestDependencies>> inProcessRequests = std::queue<std::unique_ptr<InProcessRequestDependencies>>();
        
        vk::Device& device;
        vk::CommandPool transferPool; 
        vk::PhysicalDeviceLimits deviceLimits;
        std::vector<vk::CommandBuffer> commandBuffers;
        std::vector<vk::Fence*> commandBufferFences;
        size_t previousBufferIndexUsed = 0; 
        std::vector<vk::Queue> transferQueues;
        Allocator& allocator; 

        boost::atomic<bool> shouldRun = false;
        boost::thread thread;

        static std::vector<vk::Queue> createTransferQueues(star::StarDevice& device, const uint32_t& dedicatedTransferQueueFamilyIndex); 

    };

    class TransferWorker {
    public:
        /// @brief Creates a transfer worker which owns a DEDICATED transfer queue. Will allow asynchronous transfers
        /// @param dedicatedTransferQueueFamilyIndex The queue family index which will be ONLY used in the threads spawned by this class
        /// @param device Created star device from which vulkan objects can be made
        TransferWorker(star::StarDevice& device, star::Allocator& allocator); 
        /// @brief Creates a transfer worker which does not own the dedicated transfer queue. Device might not support dedicated transfer queue. As such, transfers are submitted on the main thread.
        /// @param device Created star device from which vulkan objects can be made
        TransferWorker(StarDevice& device, star::Allocator& allocator,
            vk::Queue sharedTransferQueue, vk::CommandPool sharedCommandPool);

        void add(BufferManagerRequest& newBufferRequest, vk::Fence* workCompleteFence, 
            std::unique_ptr<StarBuffer>& resultingBuffer, const bool& isHighPriority);

        void update(); 
        
        ~TransferWorker(); 

        bool isAsync() { return asyncMode; }
    private:
        std::unique_ptr<TransferManagerThread> manager;

        /// Determines if this worker will run in an asynchronous mode
        const bool asyncMode = false;
    };

}
